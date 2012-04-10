/**
* @file		ChatDlg.h
* @brief	����Ի�����
*
* @author	��а
* @date		2012/04/08
**/

#ifndef SHUZHAN_PRACTICE_C_WUXIE_CHATDLG_H
#define SHUZHAN_PRACTICE_C_WUXIE_CHATDLG_H

#pragma once

#define WM_RECVDATA WM_USER + 1
#define WM_TIMER_SENDCOUNT WM_USER + 2
#define WM_TIMER_RECVCOUNT WM_USER + 3


/** 
* @brief	�õ�������ʾ��Ϣ
* @param	INnum ��������
* @return	���ش�����ʾ�ַ���
**/
CString GetError(DWORD INerr);

/** 
* @brief	�����ļ���Ϣ
* @param	INparam ���ڶ���ָ��
* @return	0
**/
UINT SendFileSingle(LPVOID INparam);

/** 
* @brief	�����ļ�������Ϣ
* @param	INparam ���ڶ���ָ��
* @return	0
**/
UINT ListenFileTransport(LPVOID INparam);

/** 
* @brief	�����ļ���Ϣ
* @param	INparam ���ڶ���ָ��
* @return	0
**/
UINT SaveFileSingle(LPVOID INparam);

/** 
* @brief	����������Ϣ�̺߳���
* @param	INparam RecvParam�ṹָ��
* @return	0
**/
UINT RecvProc(LPVOID INparam);

/** 
* @class	CChatDlg �Ի�����
* @brief	����������Ի���Ŀؼ����¼����Լ���������
* 
* @note		������ʼ�����̡���ʾ���̡��������̵ȷ���
**/
class CChatDlg : public CDialog
{
public:
	// �Ի�������
	enum 
	{ 
		IDD = IDD_CHAT_DIALOG 
	};
	/** ��ʾIP��listctrl�ؼ� **/
	CListCtrl m_listIp;

	/** ��ʾ�ļ����ͽ������Ŀؼ� **/
	CProgressCtrl m_progress;

	/** ��ʾ�ļ������ٶȵľ�̬��ʾ�ؼ� **/
	CStatic m_speed;

	/** �����ļ��İ�ť�ؼ� **/
	CButton m_btnSendFile;

	/** �˳�����İ�ť�ؼ� **/
	CButton m_btnQuit;

private:
	/** ��Ϣ�ӷ��׽��� **/
	SOCKET m_socket;

public:
	/** 
	* @brief	��׼���캯��
	**/
	CChatDlg(CWnd* pParent = NULL);

	/** 
	* @brief	��ʼ���׽���
	**/	
	BOOL InitSocket();

	/** 
	* @brief	�õ�CString���͵�IP
	* @param	OUTname ���Ա���������
	* @param	OUTaddr ���Ա���IP��ַ
	* @return	true �ɹ����IP false���IP��ַʧ��
	**/
	bool GetIpAddr(CString& OUTname, CString& OUTaddr);

	/** 
	* @brief	�õ�CString���͵�IP
	* @param	OUTaddr ���Ա���IP��ַ
	* @return	true �ɹ����IP false���IP��ַʧ��
	**/
	bool GetIpAddr(CString& OUTaddr);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	// ����������Ϣ��Ϣ����
	afx_msg LRESULT OnRecvData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	// ����������Ϣ����
	afx_msg void OnBnClickedSend();
	// ˢ��IP��ַ�б���Ϣ����
	afx_msg void OnBnClickedBtnRef();
	// �����ļ���Ϣ����
	afx_msg void OnBnClickedBtnSendFile();
	// ����˳���ť��Ϣ����
	afx_msg void OnBnClickedBtnExit();
};

#endif