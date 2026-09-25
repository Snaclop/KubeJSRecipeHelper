#pragma once
#include "afxdialogex.h"


// CDlgShaped 对话框

class CDlgShaped : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgShaped)

public:
	CDlgShaped(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgShaped();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSHAPED };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
