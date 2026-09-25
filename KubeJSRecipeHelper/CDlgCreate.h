#pragma once
#include "afxdialogex.h"


// CDlgCreate 对话框

class CDlgCreate : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgCreate)

public:
	CDlgCreate(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgCreate();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGCREATE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
