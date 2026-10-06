#pragma once
#include "afxdialogex.h"


// CDlgOther 对话框

class CDlgOther : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgOther)

public:
	CDlgOther(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgOther();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGOTHER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
