// CDlgCook.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgCook.h"


// CDlgCook 对话框

IMPLEMENT_DYNAMIC(CDlgCook, CDialogEx)

CDlgCook::CDlgCook(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGCOOK, pParent)
{

}

CDlgCook::~CDlgCook()
{
}

void CDlgCook::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgCook, CDialogEx)
END_MESSAGE_MAP()


// CDlgCook 消息处理程序
