
/**
* @file		Farrive.h
* @brief	�Ƿ�����ļ��Ի���
*
* @author	��а
* @date		2012/04/08
**/

#ifndef SHUZHAN_PRACTICE_C_WUXIE_FARRIVE_H
#define SHUZHAN_PRACTICE_C_WUXIE_FARRIVE_H



#pragma once


// CFArrive �Ի���

class CFArrive : public CDialog
{
	DECLARE_DYNAMIC(CFArrive)

public:
	CFArrive(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFArrive();
	
	/** �ļ��� **/
	CString m_fileName;
// �Ի�������
	enum { IDD = IDD_FARRIVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
#endif