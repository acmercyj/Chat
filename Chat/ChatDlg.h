/**
* @file		ChatDlg.h
* @brief	聊天对话框类
*
* @author	无邪
* @date		2012/04/08
**/

#ifndef SHUZHAN_PRACTICE_C_WUXIE_CHATDLG_H
#define SHUZHAN_PRACTICE_C_WUXIE_CHATDLG_H

#pragma once

#define WM_RECVDATA WM_USER + 1
#define WM_TIMER_SENDCOUNT WM_USER + 2
#define WM_TIMER_RECVCOUNT WM_USER + 3


/** 
* @brief	得到错误提示信息
* @param	INnum 白棋数量
* @return	返回错误提示字符串
**/
CString GetError(DWORD INerr);

/** 
* @brief	发送文件信息
* @param	INparam 窗口对象指针
* @return	0
**/
UINT SendFileSingle(LPVOID INparam);

/** 
* @brief	监听文件传送信息
* @param	INparam 窗口对象指针
* @return	0
**/
UINT ListenFileTransport(LPVOID INparam);

/** 
* @brief	发送文件信息
* @param	INparam 窗口对象指针
* @return	0
**/
UINT SaveFileSingle(LPVOID INparam);

/** 
* @brief	接收文字信息线程函数
* @param	INparam RecvParam结构指针
* @return	0
**/
UINT RecvProc(LPVOID INparam);

/** 
* @class	CChatDlg 对话框类
* @brief	定义了聊天对话框的控件，事件，以及其他方法
* 
* @note		包含初始化棋盘、显示棋盘、更新棋盘等方法
**/
class CChatDlg : public CDialog
{
public:
	// 对话框数据
	enum 
	{ 
		IDD = IDD_CHAT_DIALOG 
	};
	/** 显示IP的listctrl控件 **/
	CListCtrl m_listIp;

	/** 显示文件发送进度条的控件 **/
	CProgressCtrl m_progress;

	/** 显示文件发送速度的静态显示控件 **/
	CStatic m_speed;

	/** 发送文件的按钮控件 **/
	CButton m_btnSendFile;

	/** 退出程序的按钮控件 **/
	CButton m_btnQuit;

private:
	/** 消息接发套接字 **/
	SOCKET m_socket;

public:
	/** 
	* @brief	标准构造函数
	**/
	CChatDlg(CWnd* pParent = NULL);

	/** 
	* @brief	初始化套接字
	**/	
	BOOL InitSocket();

	/** 
	* @brief	得到CString类型的IP
	* @param	OUTname 用以保存主机名
	* @param	OUTaddr 用以保存IP地址
	* @return	true 成功获得IP false获得IP地址失败
	**/
	bool GetIpAddr(CString& OUTname, CString& OUTaddr);

	/** 
	* @brief	得到CString类型的IP
	* @param	OUTaddr 用以保存IP地址
	* @return	true 成功获得IP false获得IP地址失败
	**/
	bool GetIpAddr(CString& OUTaddr);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	// 接收文字信息消息函数
	afx_msg LRESULT OnRecvData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	// 发送文字消息函数
	afx_msg void OnBnClickedSend();
	// 刷新IP地址列表消息函数
	afx_msg void OnBnClickedBtnRef();
	// 发送文件消息函数
	afx_msg void OnBnClickedBtnSendFile();
	// 点击退出按钮消息函数
	afx_msg void OnBnClickedBtnExit();
};

#endif