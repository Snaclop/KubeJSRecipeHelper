#pragma once
#include "afxdialogex.h"


// CDlgSmith 对话框

class CDlgSmith : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSmith)

public:
	CDlgSmith(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgSmith();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSMITH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
