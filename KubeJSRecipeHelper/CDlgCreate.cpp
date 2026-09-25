// CDlgCreate.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgCreate.h"


// CDlgCreate 对话框

IMPLEMENT_DYNAMIC(CDlgCreate, CDialogEx)

CDlgCreate::CDlgCreate(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGCREATE, pParent)
{

}

CDlgCreate::~CDlgCreate()
{
}

void CDlgCreate::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgCreate, CDialogEx)
END_MESSAGE_MAP()


// CDlgCreate 消息处理程序
