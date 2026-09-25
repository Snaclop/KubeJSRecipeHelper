#pragma once
#include "afxdialogex.h"


// CDlgCook 对话框

class CDlgCook : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgCook)

public:
	CDlgCook(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgCook();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGCOOK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
