#pragma once
#include "afxdialogex.h"


// CDlgShapeless 对话框

class CDlgShapeless : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgShapeless)

public:
	CDlgShapeless(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgShapeless();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSHAPELESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
