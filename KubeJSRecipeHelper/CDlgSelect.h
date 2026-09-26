#pragma once
#include "afxdialogex.h"


// CDlgSelect 对话框

class CDlgSelect : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSelect)

public:
	CDlgSelect(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgSelect();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSELECT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CSpinButtonCtrl m_Spin;
};
