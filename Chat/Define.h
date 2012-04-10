/**
* @file		Define.h
* @brief	预定义结构信息
*
* @author	无邪
* @date		2012/04/08
**/

#ifndef SHUZHAN_PRACTICE_C_WUXIE_DEFINE_H
#define SHUZHAN_PRACTICE_C_WUXIE_DEFINE_H


// 保存文件结构体
struct SaveFileParam
{
	SOCKET sock;
	// 对话框对象
	CChatDlg* pCLAN;
};

// 发送文件信息结构体
struct SendFileParam
{
	// 文件路径 
	CString path;
	// IP地址
	CString ip;
	// 文件名
	char fileName[1024];
	// 对话框对象
	CChatDlg* pCLAN;
};
// 接收消息结构
struct RecvParam
{
	SOCKET sock;
	HWND hwnd;
};
#endif