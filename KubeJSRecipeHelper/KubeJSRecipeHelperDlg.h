
// KubeJSRecipeHelperDlg.h: 头文件
//

#pragma once

#include "CDlgCook.h"
#include "CDlgCreate.h"
#include "CDlgShaped.h"
#include "CDlgShapeless.h"
#include "CDlgSmith.h"
#include "CDlgStonecut.h"

// CKubeJSRecipeHelperDlg 对话框
class CKubeJSRecipeHelperDlg : public CDialogEx
{
// 构造
public:
	CKubeJSRecipeHelperDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KUBEJSRECIPEHELPER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnshaped();
	afx_msg void OnBnClickedBtnshapeless();
	afx_msg void OnBnClickedBtnsmith();
	afx_msg void OnBnClickedBtncook();
	afx_msg void OnBnClickedBtnstonecut();
	afx_msg void OnBnClickedBtncreate();
	afx_msg void OnBnClickedBtnimport();

	// 解析 JAR 文件，收集 assets/<命名空间>/models/item 与 models/block 下的所有物品、方块 id。
	// 成功时通过 pNames 返回动态分配的 CString 数组，调用方负责用 delete[] 释放；
	// 返回名称个数，失败返回 -1。
	int ParseJarModels(LPCTSTR lpszJarPath, CString*& pNames);

	// 累积保存历次导入得到的所有名称
	CStringArray m_arrNames;

	// 把 m_arrNames 的内容刷新显示到输出框
	void RefreshNamesOutput();
	afx_msg void OnBnClickedBtnclear();
	afx_msg void OnBnClickedBtndwnload();
	CString m_strOutput;
};
