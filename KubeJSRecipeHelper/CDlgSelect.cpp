// CDlgSelect.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgSelect.h"


// CDlgSelect 对话框

IMPLEMENT_DYNAMIC(CDlgSelect, CDialogEx)

CDlgSelect::CDlgSelect(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGSELECT, pParent)
{

}

CDlgSelect::~CDlgSelect()
{
}

void CDlgSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN1, m_Spin);
}

BOOL CDlgSelect::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_Spin.SetRange(1, 64);
	m_Spin.SetPos(1);
	return TRUE;
}
BEGIN_MESSAGE_MAP(CDlgSelect, CDialogEx)
END_MESSAGE_MAP()


// CDlgSelect 消息处理程序
