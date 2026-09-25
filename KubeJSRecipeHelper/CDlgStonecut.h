#pragma once
#include "afxdialogex.h"


// CDlgStonecut 对话框

class CDlgStonecut : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgStonecut)

public:
	CDlgStonecut(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgStonecut();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSTONECUT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
