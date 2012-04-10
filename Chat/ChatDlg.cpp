// ChatDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Chat.h"
#include "ChatDlg.h"
#include "FArrive.h"
#include "Define.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mpr.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 控制只有一个接收文件线程占用公共资源
HANDLE g_hEventRecv = NULL;
// 控制只有一个发送文件线程占用公共资源
HANDLE g_hEventSend = NULL;
// 已发送字节数
DWORD g_dwSendCnt = 0;
// 已接收字节数
DWORD g_dwRecvCnt = 0;
// 已发送时间
int g_sendSecond = 0;
// 已接收时间
int g_recvSecond = 0;
// 发送文件临界区
CRITICAL_SECTION g_criticalSend;
// 接收文件临界区
CRITICAL_SECTION g_criticalRecv;

const int BUFFERSIZE = 65000;
const int FILENAMELEN = 1024;
const int SENDPORT = 5050;
const int RECVPORT = 5051;
const int FILEPORT = 5052;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_LIST_IP, m_listIp);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CChatDlg 对话框




CChatDlg::CChatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChatDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_IP, m_listIp);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Control(pDX, IDC_STATIC_SPEED, m_speed);
	DDX_Control(pDX, IDC_BTN_SEND_FILE, m_btnSendFile);
}

BEGIN_MESSAGE_MAP(CChatDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDSEND, &CChatDlg::OnBnClickedSend)
	ON_MESSAGE(WM_RECVDATA, &CChatDlg::OnRecvData)
	
	ON_BN_CLICKED(IDC_BTN_REF, &CChatDlg::OnBnClickedBtnRef)
	ON_BN_CLICKED(IDC_BTN_SEND_FILE, &CChatDlg::OnBnClickedBtnSendFile)
	ON_BN_CLICKED(IDC_BTN_EXIT, &CChatDlg::OnBnClickedBtnExit)
END_MESSAGE_MAP()


// CChatDlg 消息处理程序

BOOL CChatDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	
	// 初始化接收文件临界区对象
	InitializeCriticalSection(&g_criticalRecv);

	// 初始化发送文件临界区对象
	InitializeCriticalSection(&g_criticalSend);
	
	// 创建接收文件事件对象
	g_hEventRecv = CreateEvent(NULL, false, false, "ListenConnect");
	if(!g_hEventRecv)
	{
		AfxMessageBox("创建事件对象失败！");
		exit(0);
	}
	// 判断是否有已经有程序实例在运行
	if(ERROR_ALREADY_EXISTS == GetLastError())
	{
		AfxMessageBox("已经有一个应用程序实例在运行！");
		ResetEvent(g_hEventRecv);
		exit(0);
	}

	// 创建发送文件事件对象
	g_hEventSend = CreateEvent(NULL, false, false, NULL);
	if(!g_hEventSend)
	{
		AfxMessageBox("创建事件对象失败！");
		exit(0);
	}
	if(!SetEvent(g_hEventSend))
	{
		AfxMessageBox("监听线程设置事件对象为有信号状态时出错！");
		exit(0);
	}
	// 隐藏进度条
	this->GetDlgItem(IDC_PROGRESS)->ShowWindow(SW_HIDE);
	COLORREF crf = RGB(0, 0, 255);
	this->GetDlgItem(IDC_PROGRESS)->SendMessage(PBM_SETBARCOLOR, 0, crf);

	//初始化网络库
	WSADATA wsd;
	WSAStartup(MAKEWORD(2,2),&wsd);

	// 设置IP列表风格
	m_listIp.SetExtendedStyle(LVS_EX_FLATSB
		|LVS_EX_FULLROWSELECT
		|LVS_EX_HEADERDRAGDROP
		|LVS_EX_ONECLICKACTIVATE
		|LVS_EX_GRIDLINES);

	//向表格中插入列
	m_listIp.InsertColumn(0,_TEXT("计算机名"),LVCFMT_LEFT,100,0);
	m_listIp.InsertColumn(1,_TEXT("IP地址"),LVCFMT_LEFT,100,0);
	// 初始化套接字
	InitSocket();
	RecvParam* pRecvParam = new RecvParam;
	pRecvParam->sock = m_socket;
	pRecvParam->hwnd = m_hWnd;
	// 打开接收消息线程
	AfxBeginThread(RecvProc, pRecvParam);

	// 打开监听文件传送线程
	AfxBeginThread(ListenFileTransport, this);

	::PostMessage(NULL, IDC_BTN_REF, MAKEWPARAM(IDC_BTN_REF, BN_CLICKED), NULL);
	//AfxBeginThread(RefshIpList, this);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CChatDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

void CChatDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CChatDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

UINT RecvProc(LPVOID INparam)
{
	// 得到主程序传送的套接字
	SOCKET sock=((RecvParam*)INparam)->sock;
	// 得到句柄
	HWND hwnd=((RecvParam*)INparam)->hwnd;
	// 删除该结构指针
	delete INparam;	

	SOCKADDR_IN addrFrom;

	int len = sizeof(SOCKADDR);

	char recvBuff[200];
	char tempBuff[200];
	int retval;
	
	// 循环接收文字消息
	while(true)
	{
		retval = recvfrom(sock, recvBuff, 200, 0, (SOCKADDR *)&addrFrom, &len);
		if(SOCKET_ERROR == retval)
		{
			break;
		}
		sprintf_s(tempBuff, "%s", recvBuff);
		// 发送接收文字消息  消息
		::PostMessage(hwnd, WM_RECVDATA, 0, (LPARAM)tempBuff);
	}
	return 0;
}

BOOL CChatDlg::InitSocket()
{
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(INVALID_SOCKET == m_socket)
	{
		MessageBox("套接字创建失败！");
		return false;
	}
	SOCKADDR_IN addrSock;
	// 设置地址族 端口 IP等信息
	addrSock.sin_family = AF_INET;
	addrSock.sin_port = htons(6000);
	addrSock.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	int retval;
	// 绑定
	retval = bind(m_socket, (SOCKADDR*)&addrSock, sizeof(SOCKADDR));
	if(SOCKET_ERROR == retval)
	{
		closesocket(m_socket);
		MessageBox("绑定失败！");
		return false;
	}
	return true;
}

void CChatDlg::OnBnClickedSend()
{
	CString strIP;
	CString strName;
	// 取得IP
	if(!GetIpAddr(strName, strIP))
	{
		return ;
	}
	// 转换成dword类型
	DWORD dwIP= ntohl(inet_addr(strIP));

	SOCKADDR_IN addrTo;
	// 设置地址族 端口 IP
	addrTo.sin_family = AF_INET;
	addrTo.sin_port = htons(6000);
	addrTo.sin_addr.S_un.S_addr = htonl(dwIP);

	CString strSend;
	// 取得发送内容
	GetDlgItemText(IDC_EDIT_SEND, strSend);
	CString strTemp;
	// 加上原来的字符串
	GetDlgItemText(IDC_EDIT_RECV, strTemp);                                     
	strTemp = strTemp + "我\r\n" +strSend + "\r\n";
	// 设置更新的字符串
	SetDlgItemText(IDC_EDIT_RECV, strTemp);

	// 发送的字符串前面加上主机名
	strSend = strName + "\r\n" + strSend;
	// 发送
	sendto(m_socket, strSend, strSend.GetLength() + 1, 0, 
		(SOCKADDR*)&addrTo, sizeof(SOCKADDR));

	

	// 清空编辑框
	SetDlgItemText(IDC_EDIT_SEND, "");
}

LRESULT CChatDlg::OnRecvData(WPARAM wParam, LPARAM lParam)
{
	CString str = (char *)lParam;
	// 在字符串后面加回车符
	str += _TEXT("\r\n");
	CString strTemp;
	// 加上原来的字符串
	GetDlgItemText(IDC_EDIT_RECV, strTemp);                                     
	str = strTemp + str;
	// 设置更新的字符串
	SetDlgItemText(IDC_EDIT_RECV, str);
	return true;
}
void CChatDlg::OnBnClickedBtnRef()
{
	// 删除所有主机
	m_listIp.DeleteAllItems();
	DWORD Count = 0xFFFFFFFF;
	DWORD Bufsize = 4096;
	DWORD Res;
	NETRESOURCE* nRes; 
	NETRESOURCE* nRes1; 
	NETRESOURCE* nRes2; 
	HANDLE lphEnum;
	LPVOID Buf = new char[4096];
	LPVOID Buf1 = new char[4096];
	LPVOID Buf2 = new char[4096];
	//打开列举的网络资源
	Res = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
		RESOURCEUSAGE_CONTAINER,NULL,&lphEnum);
	//获取列举的网络资源信息
	Res = WNetEnumResource(lphEnum,&Count,Buf,&Bufsize);
	nRes = (NETRESOURCE*)Buf;
	for(DWORD n = 0; n < Count; n++, nRes++)
	{
		DWORD Count1 = 0xFFFFFFFF;
		Res = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 
			RESOURCEUSAGE_CONTAINER,nRes,&lphEnum);
		Res = WNetEnumResource(lphEnum,&Count1,Buf1,&Bufsize);
		nRes1 = (NETRESOURCE*)Buf1;
		for(DWORD i = 0; i < Count1; i++, nRes1++)
		{
			DWORD Count2 = 0xFFFFFFFF;
			Res = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 
				RESOURCEUSAGE_CONTAINER,nRes1,&lphEnum);
			Res = WNetEnumResource(lphEnum,&Count2,Buf2,&Bufsize);
			nRes2 = (NETRESOURCE*)Buf2;
			for(DWORD j = 0; j < Count2; j++, nRes2++)
			{
				m_listIp.InsertItem(j, 0);
				CString sName = nRes2->lpRemoteName;
				sName = sName.Right(sName.GetLength()-2);
				m_listIp.SetItemText(j, 0, sName);
				CString str = _TEXT("");
				struct hostent* pHost;
				pHost = gethostbyname(sName);
				if(NULL == pHost)
				{
					m_listIp.SetItemText(j, 1, _TEXT("无法获得IP地址"));
				}
				else
				{
					for(int n = 0; n < 4; n++)
					{
						CString addr;
						if(n > 0)
						{
							str += ".";
						}
						addr.Format("%u", (unsigned int)((unsigned char*)pHost->h_addr_list[0])[n]);
						str += addr;
					}
					m_listIp.SetItemText(j, 1, str);
				}
			}
		}
	}
	delete Buf;
	delete Buf1;
	delete Buf2;
	WNetCloseEnum(lphEnum);
}



void CChatDlg::OnBnClickedBtnSendFile()
{
	CString ip;
	if(!GetIpAddr(ip))
	{
		return ;
	}
	// 记录返回对话框的结果
	int modal;
	int nCount;
	CString fileName;
	// 打开文件对话框
	CFileDialog fdlg(true);
	modal = (int)fdlg.DoModal();
	// 取消选择文件 则返回
	if(IDCANCEL == modal)
	{
		return ;
	}
	SendFileParam* p = new SendFileParam;
	// 得到文件名 路径 名字长度
	fileName = fdlg.GetFileName();
	p->path = fdlg.GetPathName();
	nCount = fileName.GetLength();

	memset(p->fileName, 0, FILENAMELEN);
	for(int i = 0; i < nCount; ++i)
	{
		p->fileName[i] = fileName.GetAt(i);
	}
	p->pCLAN = this;
	p->ip = ip;
	// 启动发送文件线程
	::AfxBeginThread(SendFileSingle, (LPVOID)p);
	return ;
}

bool CChatDlg::GetIpAddr(CString& OUTname, CString& OUTaddr)
{
	// 是都选择了 聊天主机
	bool isChoosed = false;
	// 选择的list索引
	int listIndex = -1;
	// 得到IP列表主机数量
	int listCount = this->m_listIp.GetItemCount();
	for(int i = 0; i < listCount; ++i)
	{
		// 如果发现选择的项 则取得该项索引
		if(this->m_listIp.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			isChoosed = true;
			listIndex = i;
			break;
		}
	}
	// 没有选择返回提示
	if(!isChoosed)
	{
		AfxMessageBox("你没有选择任何主机，请选择.");
		return false;
	}
	// 设置IP
	OUTaddr = this->m_listIp.GetItemText(listIndex, 1);
	// 设置主机名
	OUTname = this->m_listIp.GetItemText(listIndex, 0);
	return true;
}

bool CChatDlg::GetIpAddr(CString& OUTaddr)
{
	// 是都选择了 聊天主机
	bool isChoosed = false;
	// 选择的list索引
	int listIndex = -1;
	// 得到IP列表主机数量
	int listCount = this->m_listIp.GetItemCount();
	for(int i = 0; i < listCount; ++i)
	{
		// 如果发现选择的项 则取得该项索引
		if(this->m_listIp.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			isChoosed = true;
			listIndex = i;
			break;
		}
	}
	// 没有选择返回提示
	if(!isChoosed)
	{
		AfxMessageBox("你没有选择任何主机，请选择.");
		return false;
	}
	// 赋值IP
	OUTaddr = this->m_listIp.GetItemText(listIndex, 1);
	return true;
}

void CChatDlg::OnBnClickedBtnExit()
{
	// 重置事件对象
	ResetEvent(g_hEventRecv);
	ResetEvent(g_hEventSend);
	// 删除临界区资源
	DeleteCriticalSection(&g_criticalRecv);
	DeleteCriticalSection(&g_criticalSend);
	CDialog::OnCancel();
}	

UINT SendFileSingle(LPVOID INparam)
{
	if(!AfxSocketInit())
	{
		AfxMessageBox("发送文件线程初始化失败！");
		return 0;
	}

	if(WAIT_FAILED == WaitForSingleObject(g_hEventSend, INFINITE))
	{
		return 0;
	}
	CChatDlg *pCLAN = ((SendFileParam *)INparam)->pCLAN;
	CString ip;
	// 文件名
	char fileName[FILENAMELEN];
	// 文件内容临时存放区
	char buffer[BUFFERSIZE];
	DWORD length;
	CFile file;
	memset(fileName, 0, FILENAMELEN);
	strcpy_s(fileName, ((SendFileParam *)INparam)->fileName);
	ip = ((SendFileParam *)INparam)->ip;
	// 打开文件
	if(0 == file.Open(((SendFileParam *)INparam)->path, CFile::modeRead|CFile::typeBinary))
	{
		AfxMessageBox("打开文件出错！");
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		return 0;
	}
	length = (DWORD)file.GetLength();

	CSocket sockSend;
	if(0 == sockSend.Create())
	{
		AfxMessageBox("创建发送套接字失败！");

		// 重置对象事件为有信号状态，以便下次发送
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		return 0;
	}
	if(0 == sockSend.Connect(ip, FILEPORT))
	{
		AfxMessageBox("发送文件链接错误：" + GetError(GetLastError()));
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		sockSend.Close();
		return 0;
	}
	if(SOCKET_ERROR == sockSend.Send(&length, sizeof(DWORD)))
	{
		AfxMessageBox("发送文件长度出错：" + GetError(GetLastError()));
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		sockSend.Close();
		return 0;
	}
	if(SOCKET_ERROR == sockSend.Send(fileName, FILENAMELEN))
	{
		AfxMessageBox("发送文件名出错：" + GetError(GetLastError()));
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		sockSend.Close();
		return 0;
	}
	DWORD step = 0;
	int over;
	int err;
	// 显示进度条
	((CWnd *)pCLAN->GetDlgItem(IDC_PROGRESS))->ShowWindow(SW_SHOW);
	pCLAN->m_progress.SetRange32(0, length);
	pCLAN->m_progress.SetPos(0);
	// 进入发送临界区
	EnterCriticalSection(&g_criticalSend);
	g_sendSecond = 1;
	LeaveCriticalSection(&g_criticalSend);
	// 显示发送速度
	pCLAN->m_speed.ShowWindow(SW_SHOW);
	pCLAN->SetTimer(WM_TIMER_SENDCOUNT, 1000, NULL);
	// 发送文件内容
	while(true)
	{
		// 重定向文件指针到 文件最前端
		file.Seek(step, CFile::begin);
		// 读取文件内容
		over = file.Read(buffer, BUFFERSIZE);
		// 发送文件内容
		err = sockSend.Send(buffer, over);
		// 如果文件发送完毕跳出循环
		if(BUFFERSIZE > over)
		{
			
			break;
		}
		// 套接字错误则 还原控件 关闭套接字 返回
;		if(SOCKET_ERROR == err)
		{
			AfxMessageBox("发送文件内容出错：" + GetError(GetLastError()));
			if(!SetEvent(g_hEventSend))
			{
				return 0;
			}
			file.Close();
			sockSend.Close();
			pCLAN->GetDlgItem(IDC_PROGRESS)->ShowWindow(SW_HIDE);
			pCLAN->KillTimer(WM_TIMER_SENDCOUNT);
			pCLAN->m_speed.ShowWindow(SW_HIDE);
			return 0;
		}
		step += err;
		EnterCriticalSection(&g_criticalSend);
		g_dwSendCnt = step;
		LeaveCriticalSection(&g_criticalSend);
		pCLAN->m_progress.SetPos(step);
		pCLAN->m_progress.Invalidate(false);
	}
	while(true)
	{
		sockSend.Receive(buffer, 7);
		if(strcmp("finish", buffer) == 0)
		{
			break;
		}

	}
	// 关闭文件指针
	file.Close();
	// 关闭套接字
	sockSend.Close();
	// 隐藏进度条
	pCLAN->m_progress.SetPos(0);
	pCLAN->GetDlgItem(IDC_PROGRESS)->ShowWindow(SW_HIDE);
	// 关闭定时器
	pCLAN->KillTimer(WM_TIMER_SENDCOUNT);
	// 隐藏速度显示
	pCLAN->m_speed.ShowWindow(SW_HIDE);
	if(!SetEvent(g_hEventSend))
	{
		return 0;
	}
	CString strFinish;
	strFinish.Format(" %s 发送成功", fileName);
	AfxMessageBox(strFinish);
	delete (SendFileParam *)INparam;
	return 0;
}

CString GetError(DWORD INerr)
{
	CString strErr;
	// 根据错误代码 返回错误信息
	switch(INerr)
	{
	case WSANOTINITIALISED:
		strErr = "初始化错误";
		break;
	case WSAENOTCONN:
		strErr = "对方没有启动";
		break;
	case WSAEWOULDBLOCK:
		strErr = "对方已关闭";
		break;
	case WSAECONNREFUSED:
		strErr = "连接被拒绝";
		break;
	case WSAENOTSOCK:
		strErr = "在一个非套接字上尝试了一个操作";
		break;
	case WSAEADDRINUSE:
		strErr = "待定的地址已在使用中";
		break;
	case WSAECONNRESET:
		strErr = "与主机的连接被关闭";
		break;
	default :
		strErr = "其他错误";
		break;
	}
	return strErr;
}

UINT SaveFileSingle(LPVOID INparam)
{
	SaveFileParam sp;
	sp.pCLAN = ((SaveFileParam *)INparam)->pCLAN;
	sp.sock = ((SaveFileParam *)INparam)->sock;

	CSocket sock;
	// 取得sp.sock的句柄
	sock.Attach(sp.sock);

	DWORD length;
	// 文件名
	char fileName[FILENAMELEN];
	// 文件路径
	CString savePathName;
	int modal;
	int err;
	memset(fileName, 0, FILENAMELEN);
	err = sock.Receive(&length, sizeof(DWORD));
	if(0 == err)
	{
		AfxMessageBox("连接已断开!");
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	if(SOCKET_ERROR == err)
	{
		AfxMessageBox(GetError(GetLastError()));
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}

	// 接收文件名
	err = sock.Receive(fileName, FILENAMELEN);
	if(0 == err)
	{
		AfxMessageBox("连接已断开!");
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	if(SOCKET_ERROR == err)
	{
		AfxMessageBox(GetError(GetLastError()));
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	// 是否保存文件提示对话框
	CFArrive dlg;
	dlg.m_fileName.Format("文件: %s 到达\r\n是否接收？",fileName);
	if(IDCANCEL == dlg.DoModal())
	{
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	// 保存文件对话框
	CFileDialog fdlg(false, NULL, fileName);
	modal = (int)fdlg.DoModal();
	if(IDCANCEL == modal)
	{
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	savePathName = fdlg.GetPathName();

	int finish = 0;
	DWORD step = 0;
	CFile file;
	char buffer[BUFFERSIZE];
	CProgressCtrl* pProgress;
	if(0 == file.Open(savePathName, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary))
	{
		AfxMessageBox("创建写入文件出错！");
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	pProgress = (CProgressCtrl *)sp.pCLAN->GetDlgItem(IDC_PROGRESS);
	pProgress->ShowWindow(SW_SHOW);
	pProgress->SetRange32(0, length);
	pProgress->SetPos(0);

	EnterCriticalSection(&g_criticalRecv);
	g_recvSecond = 1;
	LeaveCriticalSection(&g_criticalRecv);

	sp.pCLAN->m_speed.ShowWindow(SW_SHOW);
	sp.pCLAN->SetTimer(WM_TIMER_RECVCOUNT, 1000, NULL);
	// 循环接收文件内容并写入文件
	while(true)
	{
		// 开始写入
		finish = sock.Receive(buffer, BUFFERSIZE);
		// 文件传送完毕 跳出
		if(0 == finish)
		{
			// 反馈接收完成消息
			break;
		}
		
		if(SOCKET_ERROR == finish)
		{
			AfxMessageBox( GetError(GetLastError()));
			sock.Close();
			sp.pCLAN->m_speed.ShowWindow(SW_HIDE);
			sp.pCLAN->KillTimer(WM_TIMER_RECVCOUNT);
			if(!SetEvent(g_hEventRecv))
			{
				return 0;
			}
			return 0;
		}
		// 写入文件内容
		file.Write(buffer, finish);
		step += finish;
		
		EnterCriticalSection(&g_criticalRecv);
		g_dwRecvCnt = step;
		LeaveCriticalSection(&g_criticalRecv);
		pProgress->SetPos(step);
		pProgress->Invalidate(false);
		// 文件发送完毕则发送 完成反馈信息
		if(BUFFERSIZE > finish)
		{
			char revFinish[7]={"finish"};
			sock.Send(revFinish, 7);
			break;
		}
	}
	
	// 关闭文件
	file.Close();
	// 关闭套接字
	sock.Close();
	// 隐藏进度条
	pProgress->SetPos(0);
	pProgress->ShowWindow(SW_HIDE);
	sp.pCLAN->KillTimer(WM_TIMER_RECVCOUNT);
	if(!SetEvent(g_hEventRecv))
	{
		return 0;
	}
	// 记录接收成功信息
	CString strFinish;
	strFinish.Format("文件： %s 已经接收成功", fileName);
	AfxMessageBox(strFinish);

	return 0;
}
UINT ListenFileTransport(LPVOID INparam)
{
	CChatDlg* pCLAN = (CChatDlg *)INparam;
	CSocket sockServer;
	if(!AfxSocketInit())
	{
		AfxMessageBox("监听文件传输链接线程失败！");
		return 0;
	}
	if(0 == sockServer.Create(FILEPORT))
	{
		AfxMessageBox("监听套接字创建失败！");
		return 0;
	}
	if(0 == sockServer.Listen())
	{
		AfxMessageBox("监听失败！");
		sockServer.Close();
		return 0;
	}
	if(!SetEvent(g_hEventRecv))
	{
		//AfxMessageBox("监听线程设置事件对象为有信号状态时出错！");
		sockServer.Close();
		return 0;
	}
	// 监听套接字创建成功发送文件按钮可用
	pCLAN->m_btnSendFile.EnableWindow(true);
	while(true)
	{
		CSocket sockClient;
		if(0 == sockServer.Accept(sockClient))
		{
			AfxMessageBox("监听套接字接收服务失败！");
			sockServer.Close();
			break;
		}
		SaveFileParam param;
		param.sock = sockClient.Detach();
		param.pCLAN = pCLAN;

		if(WAIT_FAILED == WaitForSingleObject(g_hEventRecv, INFINITE))
		{
			AfxMessageBox("等待事件对象错误！");
			sockServer.Close();
			return 0;
		}
		// 启动保存文件线程
		AfxBeginThread(SaveFileSingle, (LPVOID)&param);
	}
	sockServer.Close();
	return 0;
}

