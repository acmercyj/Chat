/**
* @file		Define.h
* @brief	Ԥ����ṹ��Ϣ
*
* @author	��а
* @date		2012/04/08
**/

#ifndef SHUZHAN_PRACTICE_C_WUXIE_DEFINE_H
#define SHUZHAN_PRACTICE_C_WUXIE_DEFINE_H


// �����ļ��ṹ��
struct SaveFileParam
{
	SOCKET sock;
	// �Ի������
	CChatDlg* pCLAN;
};

// �����ļ���Ϣ�ṹ��
struct SendFileParam
{
	// �ļ�·�� 
	CString path;
	// IP��ַ
	CString ip;
	// �ļ���
	char fileName[1024];
	// �Ի������
	CChatDlg* pCLAN;
};
// ������Ϣ�ṹ
struct RecvParam
{
	SOCKET sock;
	HWND hwnd;
};
#endif