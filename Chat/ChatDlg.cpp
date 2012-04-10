// ChatDlg.cpp : ʵ���ļ�
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

// ����ֻ��һ�������ļ��߳�ռ�ù�����Դ
HANDLE g_hEventRecv = NULL;
// ����ֻ��һ�������ļ��߳�ռ�ù�����Դ
HANDLE g_hEventSend = NULL;
// �ѷ����ֽ���
DWORD g_dwSendCnt = 0;
// �ѽ����ֽ���
DWORD g_dwRecvCnt = 0;
// �ѷ���ʱ��
int g_sendSecond = 0;
// �ѽ���ʱ��
int g_recvSecond = 0;
// �����ļ��ٽ���
CRITICAL_SECTION g_criticalSend;
// �����ļ��ٽ���
CRITICAL_SECTION g_criticalRecv;

const int BUFFERSIZE = 65000;
const int FILENAMELEN = 1024;
const int SENDPORT = 5050;
const int RECVPORT = 5051;
const int FILEPORT = 5052;

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CChatDlg �Ի���




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


// CChatDlg ��Ϣ�������

BOOL CChatDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��
	
	// ��ʼ�������ļ��ٽ�������
	InitializeCriticalSection(&g_criticalRecv);

	// ��ʼ�������ļ��ٽ�������
	InitializeCriticalSection(&g_criticalSend);
	
	// ���������ļ��¼�����
	g_hEventRecv = CreateEvent(NULL, false, false, "ListenConnect");
	if(!g_hEventRecv)
	{
		AfxMessageBox("�����¼�����ʧ�ܣ�");
		exit(0);
	}
	// �ж��Ƿ����Ѿ��г���ʵ��������
	if(ERROR_ALREADY_EXISTS == GetLastError())
	{
		AfxMessageBox("�Ѿ���һ��Ӧ�ó���ʵ�������У�");
		ResetEvent(g_hEventRecv);
		exit(0);
	}

	// ���������ļ��¼�����
	g_hEventSend = CreateEvent(NULL, false, false, NULL);
	if(!g_hEventSend)
	{
		AfxMessageBox("�����¼�����ʧ�ܣ�");
		exit(0);
	}
	if(!SetEvent(g_hEventSend))
	{
		AfxMessageBox("�����߳������¼�����Ϊ���ź�״̬ʱ����");
		exit(0);
	}
	// ���ؽ�����
	this->GetDlgItem(IDC_PROGRESS)->ShowWindow(SW_HIDE);
	COLORREF crf = RGB(0, 0, 255);
	this->GetDlgItem(IDC_PROGRESS)->SendMessage(PBM_SETBARCOLOR, 0, crf);

	//��ʼ�������
	WSADATA wsd;
	WSAStartup(MAKEWORD(2,2),&wsd);

	// ����IP�б���
	m_listIp.SetExtendedStyle(LVS_EX_FLATSB
		|LVS_EX_FULLROWSELECT
		|LVS_EX_HEADERDRAGDROP
		|LVS_EX_ONECLICKACTIVATE
		|LVS_EX_GRIDLINES);

	//�����в�����
	m_listIp.InsertColumn(0,_TEXT("�������"),LVCFMT_LEFT,100,0);
	m_listIp.InsertColumn(1,_TEXT("IP��ַ"),LVCFMT_LEFT,100,0);
	// ��ʼ���׽���
	InitSocket();
	RecvParam* pRecvParam = new RecvParam;
	pRecvParam->sock = m_socket;
	pRecvParam->hwnd = m_hWnd;
	// �򿪽�����Ϣ�߳�
	AfxBeginThread(RecvProc, pRecvParam);

	// �򿪼����ļ������߳�
	AfxBeginThread(ListenFileTransport, this);

	::PostMessage(NULL, IDC_BTN_REF, MAKEWPARAM(IDC_BTN_REF, BN_CLICKED), NULL);
	//AfxBeginThread(RefshIpList, this);
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
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
	// �õ��������͵��׽���
	SOCKET sock=((RecvParam*)INparam)->sock;
	// �õ����
	HWND hwnd=((RecvParam*)INparam)->hwnd;
	// ɾ���ýṹָ��
	delete INparam;	

	SOCKADDR_IN addrFrom;

	int len = sizeof(SOCKADDR);

	char recvBuff[200];
	char tempBuff[200];
	int retval;
	
	// ѭ������������Ϣ
	while(true)
	{
		retval = recvfrom(sock, recvBuff, 200, 0, (SOCKADDR *)&addrFrom, &len);
		if(SOCKET_ERROR == retval)
		{
			break;
		}
		sprintf_s(tempBuff, "%s", recvBuff);
		// ���ͽ���������Ϣ  ��Ϣ
		::PostMessage(hwnd, WM_RECVDATA, 0, (LPARAM)tempBuff);
	}
	return 0;
}

BOOL CChatDlg::InitSocket()
{
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(INVALID_SOCKET == m_socket)
	{
		MessageBox("�׽��ִ���ʧ�ܣ�");
		return false;
	}
	SOCKADDR_IN addrSock;
	// ���õ�ַ�� �˿� IP����Ϣ
	addrSock.sin_family = AF_INET;
	addrSock.sin_port = htons(6000);
	addrSock.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	int retval;
	// ��
	retval = bind(m_socket, (SOCKADDR*)&addrSock, sizeof(SOCKADDR));
	if(SOCKET_ERROR == retval)
	{
		closesocket(m_socket);
		MessageBox("��ʧ�ܣ�");
		return false;
	}
	return true;
}

void CChatDlg::OnBnClickedSend()
{
	CString strIP;
	CString strName;
	// ȡ��IP
	if(!GetIpAddr(strName, strIP))
	{
		return ;
	}
	// ת����dword����
	DWORD dwIP= ntohl(inet_addr(strIP));

	SOCKADDR_IN addrTo;
	// ���õ�ַ�� �˿� IP
	addrTo.sin_family = AF_INET;
	addrTo.sin_port = htons(6000);
	addrTo.sin_addr.S_un.S_addr = htonl(dwIP);

	CString strSend;
	// ȡ�÷�������
	GetDlgItemText(IDC_EDIT_SEND, strSend);
	CString strTemp;
	// ����ԭ�����ַ���
	GetDlgItemText(IDC_EDIT_RECV, strTemp);                                     
	strTemp = strTemp + "��\r\n" +strSend + "\r\n";
	// ���ø��µ��ַ���
	SetDlgItemText(IDC_EDIT_RECV, strTemp);

	// ���͵��ַ���ǰ�����������
	strSend = strName + "\r\n" + strSend;
	// ����
	sendto(m_socket, strSend, strSend.GetLength() + 1, 0, 
		(SOCKADDR*)&addrTo, sizeof(SOCKADDR));

	

	// ��ձ༭��
	SetDlgItemText(IDC_EDIT_SEND, "");
}

LRESULT CChatDlg::OnRecvData(WPARAM wParam, LPARAM lParam)
{
	CString str = (char *)lParam;
	// ���ַ�������ӻس���
	str += _TEXT("\r\n");
	CString strTemp;
	// ����ԭ�����ַ���
	GetDlgItemText(IDC_EDIT_RECV, strTemp);                                     
	str = strTemp + str;
	// ���ø��µ��ַ���
	SetDlgItemText(IDC_EDIT_RECV, str);
	return true;
}
void CChatDlg::OnBnClickedBtnRef()
{
	// ɾ����������
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
	//���оٵ�������Դ
	Res = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
		RESOURCEUSAGE_CONTAINER,NULL,&lphEnum);
	//��ȡ�оٵ�������Դ��Ϣ
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
					m_listIp.SetItemText(j, 1, _TEXT("�޷����IP��ַ"));
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
	// ��¼���ضԻ���Ľ��
	int modal;
	int nCount;
	CString fileName;
	// ���ļ��Ի���
	CFileDialog fdlg(true);
	modal = (int)fdlg.DoModal();
	// ȡ��ѡ���ļ� �򷵻�
	if(IDCANCEL == modal)
	{
		return ;
	}
	SendFileParam* p = new SendFileParam;
	// �õ��ļ��� ·�� ���ֳ���
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
	// ���������ļ��߳�
	::AfxBeginThread(SendFileSingle, (LPVOID)p);
	return ;
}

bool CChatDlg::GetIpAddr(CString& OUTname, CString& OUTaddr)
{
	// �Ƕ�ѡ���� ��������
	bool isChoosed = false;
	// ѡ���list����
	int listIndex = -1;
	// �õ�IP�б���������
	int listCount = this->m_listIp.GetItemCount();
	for(int i = 0; i < listCount; ++i)
	{
		// �������ѡ����� ��ȡ�ø�������
		if(this->m_listIp.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			isChoosed = true;
			listIndex = i;
			break;
		}
	}
	// û��ѡ�񷵻���ʾ
	if(!isChoosed)
	{
		AfxMessageBox("��û��ѡ���κ���������ѡ��.");
		return false;
	}
	// ����IP
	OUTaddr = this->m_listIp.GetItemText(listIndex, 1);
	// ����������
	OUTname = this->m_listIp.GetItemText(listIndex, 0);
	return true;
}

bool CChatDlg::GetIpAddr(CString& OUTaddr)
{
	// �Ƕ�ѡ���� ��������
	bool isChoosed = false;
	// ѡ���list����
	int listIndex = -1;
	// �õ�IP�б���������
	int listCount = this->m_listIp.GetItemCount();
	for(int i = 0; i < listCount; ++i)
	{
		// �������ѡ����� ��ȡ�ø�������
		if(this->m_listIp.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			isChoosed = true;
			listIndex = i;
			break;
		}
	}
	// û��ѡ�񷵻���ʾ
	if(!isChoosed)
	{
		AfxMessageBox("��û��ѡ���κ���������ѡ��.");
		return false;
	}
	// ��ֵIP
	OUTaddr = this->m_listIp.GetItemText(listIndex, 1);
	return true;
}

void CChatDlg::OnBnClickedBtnExit()
{
	// �����¼�����
	ResetEvent(g_hEventRecv);
	ResetEvent(g_hEventSend);
	// ɾ���ٽ�����Դ
	DeleteCriticalSection(&g_criticalRecv);
	DeleteCriticalSection(&g_criticalSend);
	CDialog::OnCancel();
}	

UINT SendFileSingle(LPVOID INparam)
{
	if(!AfxSocketInit())
	{
		AfxMessageBox("�����ļ��̳߳�ʼ��ʧ�ܣ�");
		return 0;
	}

	if(WAIT_FAILED == WaitForSingleObject(g_hEventSend, INFINITE))
	{
		return 0;
	}
	CChatDlg *pCLAN = ((SendFileParam *)INparam)->pCLAN;
	CString ip;
	// �ļ���
	char fileName[FILENAMELEN];
	// �ļ�������ʱ�����
	char buffer[BUFFERSIZE];
	DWORD length;
	CFile file;
	memset(fileName, 0, FILENAMELEN);
	strcpy_s(fileName, ((SendFileParam *)INparam)->fileName);
	ip = ((SendFileParam *)INparam)->ip;
	// ���ļ�
	if(0 == file.Open(((SendFileParam *)INparam)->path, CFile::modeRead|CFile::typeBinary))
	{
		AfxMessageBox("���ļ�����");
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
		AfxMessageBox("���������׽���ʧ�ܣ�");

		// ���ö����¼�Ϊ���ź�״̬���Ա��´η���
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		return 0;
	}
	if(0 == sockSend.Connect(ip, FILEPORT))
	{
		AfxMessageBox("�����ļ����Ӵ���" + GetError(GetLastError()));
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		sockSend.Close();
		return 0;
	}
	if(SOCKET_ERROR == sockSend.Send(&length, sizeof(DWORD)))
	{
		AfxMessageBox("�����ļ����ȳ���" + GetError(GetLastError()));
		if(!SetEvent(g_hEventSend))
		{
			return 0;
		}
		sockSend.Close();
		return 0;
	}
	if(SOCKET_ERROR == sockSend.Send(fileName, FILENAMELEN))
	{
		AfxMessageBox("�����ļ�������" + GetError(GetLastError()));
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
	// ��ʾ������
	((CWnd *)pCLAN->GetDlgItem(IDC_PROGRESS))->ShowWindow(SW_SHOW);
	pCLAN->m_progress.SetRange32(0, length);
	pCLAN->m_progress.SetPos(0);
	// ���뷢���ٽ���
	EnterCriticalSection(&g_criticalSend);
	g_sendSecond = 1;
	LeaveCriticalSection(&g_criticalSend);
	// ��ʾ�����ٶ�
	pCLAN->m_speed.ShowWindow(SW_SHOW);
	pCLAN->SetTimer(WM_TIMER_SENDCOUNT, 1000, NULL);
	// �����ļ�����
	while(true)
	{
		// �ض����ļ�ָ�뵽 �ļ���ǰ��
		file.Seek(step, CFile::begin);
		// ��ȡ�ļ�����
		over = file.Read(buffer, BUFFERSIZE);
		// �����ļ�����
		err = sockSend.Send(buffer, over);
		// ����ļ������������ѭ��
		if(BUFFERSIZE > over)
		{
			
			break;
		}
		// �׽��ִ����� ��ԭ�ؼ� �ر��׽��� ����
;		if(SOCKET_ERROR == err)
		{
			AfxMessageBox("�����ļ����ݳ���" + GetError(GetLastError()));
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
	// �ر��ļ�ָ��
	file.Close();
	// �ر��׽���
	sockSend.Close();
	// ���ؽ�����
	pCLAN->m_progress.SetPos(0);
	pCLAN->GetDlgItem(IDC_PROGRESS)->ShowWindow(SW_HIDE);
	// �رն�ʱ��
	pCLAN->KillTimer(WM_TIMER_SENDCOUNT);
	// �����ٶ���ʾ
	pCLAN->m_speed.ShowWindow(SW_HIDE);
	if(!SetEvent(g_hEventSend))
	{
		return 0;
	}
	CString strFinish;
	strFinish.Format(" %s ���ͳɹ�", fileName);
	AfxMessageBox(strFinish);
	delete (SendFileParam *)INparam;
	return 0;
}

CString GetError(DWORD INerr)
{
	CString strErr;
	// ���ݴ������ ���ش�����Ϣ
	switch(INerr)
	{
	case WSANOTINITIALISED:
		strErr = "��ʼ������";
		break;
	case WSAENOTCONN:
		strErr = "�Է�û������";
		break;
	case WSAEWOULDBLOCK:
		strErr = "�Է��ѹر�";
		break;
	case WSAECONNREFUSED:
		strErr = "���ӱ��ܾ�";
		break;
	case WSAENOTSOCK:
		strErr = "��һ�����׽����ϳ�����һ������";
		break;
	case WSAEADDRINUSE:
		strErr = "�����ĵ�ַ����ʹ����";
		break;
	case WSAECONNRESET:
		strErr = "�����������ӱ��ر�";
		break;
	default :
		strErr = "��������";
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
	// ȡ��sp.sock�ľ��
	sock.Attach(sp.sock);

	DWORD length;
	// �ļ���
	char fileName[FILENAMELEN];
	// �ļ�·��
	CString savePathName;
	int modal;
	int err;
	memset(fileName, 0, FILENAMELEN);
	err = sock.Receive(&length, sizeof(DWORD));
	if(0 == err)
	{
		AfxMessageBox("�����ѶϿ�!");
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

	// �����ļ���
	err = sock.Receive(fileName, FILENAMELEN);
	if(0 == err)
	{
		AfxMessageBox("�����ѶϿ�!");
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
	// �Ƿ񱣴��ļ���ʾ�Ի���
	CFArrive dlg;
	dlg.m_fileName.Format("�ļ�: %s ����\r\n�Ƿ���գ�",fileName);
	if(IDCANCEL == dlg.DoModal())
	{
		sock.Close();
		if(!SetEvent(g_hEventRecv))
		{
			return 0;
		}
		return 0;
	}
	// �����ļ��Ի���
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
		AfxMessageBox("����д���ļ�����");
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
	// ѭ�������ļ����ݲ�д���ļ�
	while(true)
	{
		// ��ʼд��
		finish = sock.Receive(buffer, BUFFERSIZE);
		// �ļ�������� ����
		if(0 == finish)
		{
			// �������������Ϣ
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
		// д���ļ�����
		file.Write(buffer, finish);
		step += finish;
		
		EnterCriticalSection(&g_criticalRecv);
		g_dwRecvCnt = step;
		LeaveCriticalSection(&g_criticalRecv);
		pProgress->SetPos(step);
		pProgress->Invalidate(false);
		// �ļ������������ ��ɷ�����Ϣ
		if(BUFFERSIZE > finish)
		{
			char revFinish[7]={"finish"};
			sock.Send(revFinish, 7);
			break;
		}
	}
	
	// �ر��ļ�
	file.Close();
	// �ر��׽���
	sock.Close();
	// ���ؽ�����
	pProgress->SetPos(0);
	pProgress->ShowWindow(SW_HIDE);
	sp.pCLAN->KillTimer(WM_TIMER_RECVCOUNT);
	if(!SetEvent(g_hEventRecv))
	{
		return 0;
	}
	// ��¼���ճɹ���Ϣ
	CString strFinish;
	strFinish.Format("�ļ��� %s �Ѿ����ճɹ�", fileName);
	AfxMessageBox(strFinish);

	return 0;
}
UINT ListenFileTransport(LPVOID INparam)
{
	CChatDlg* pCLAN = (CChatDlg *)INparam;
	CSocket sockServer;
	if(!AfxSocketInit())
	{
		AfxMessageBox("�����ļ����������߳�ʧ�ܣ�");
		return 0;
	}
	if(0 == sockServer.Create(FILEPORT))
	{
		AfxMessageBox("�����׽��ִ���ʧ�ܣ�");
		return 0;
	}
	if(0 == sockServer.Listen())
	{
		AfxMessageBox("����ʧ�ܣ�");
		sockServer.Close();
		return 0;
	}
	if(!SetEvent(g_hEventRecv))
	{
		//AfxMessageBox("�����߳������¼�����Ϊ���ź�״̬ʱ����");
		sockServer.Close();
		return 0;
	}
	// �����׽��ִ����ɹ������ļ���ť����
	pCLAN->m_btnSendFile.EnableWindow(true);
	while(true)
	{
		CSocket sockClient;
		if(0 == sockServer.Accept(sockClient))
		{
			AfxMessageBox("�����׽��ֽ��շ���ʧ�ܣ�");
			sockServer.Close();
			break;
		}
		SaveFileParam param;
		param.sock = sockClient.Detach();
		param.pCLAN = pCLAN;

		if(WAIT_FAILED == WaitForSingleObject(g_hEventRecv, INFINITE))
		{
			AfxMessageBox("�ȴ��¼��������");
			sockServer.Close();
			return 0;
		}
		// ���������ļ��߳�
		AfxBeginThread(SaveFileSingle, (LPVOID)&param);
	}
	sockServer.Close();
	return 0;
}

