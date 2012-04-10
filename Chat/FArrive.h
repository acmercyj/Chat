
/**
* @file		Farrive.h
* @brief	是否接收文件对话框
*
* @author	无邪
* @date		2012/04/08
**/

#ifndef SHUZHAN_PRACTICE_C_WUXIE_FARRIVE_H
#define SHUZHAN_PRACTICE_C_WUXIE_FARRIVE_H



#pragma once


// CFArrive 对话框

class CFArrive : public CDialog
{
	DECLARE_DYNAMIC(CFArrive)

public:
	CFArrive(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFArrive();
	
	/** 文件名 **/
	CString m_fileName;
// 对话框数据
	enum { IDD = IDD_FARRIVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
#endif