// FArrive.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Chat.h"
#include "FArrive.h"


// CFArrive �Ի���

IMPLEMENT_DYNAMIC(CFArrive, CDialog)

CFArrive::CFArrive(CWnd* pParent /*=NULL*/)
	: CDialog(CFArrive::IDD, pParent)
{
	m_fileName = _T("");
}

CFArrive::~CFArrive()
{
}

void CFArrive::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_FILENAME, m_fileName);
}


BEGIN_MESSAGE_MAP(CFArrive, CDialog)
END_MESSAGE_MAP()


// CFArrive ��Ϣ�������
