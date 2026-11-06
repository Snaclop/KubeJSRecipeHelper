
// KubeJSRecipeHelperDlg.h: 头文件
//

#pragma once

#include "afxdialogex.h"
#include "JarImporter.h"
#include "ItemTextureStore.h"

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
	afx_msg void OnBnClickedBtnvanilla();
	afx_msg void OnBnClickedBtncreate();
	afx_msg void OnBnClickedBtnimport();

	// 累积保存历次导入得到的所有名称
	CStringArray m_arrNames;
	JarImportResult m_importResult;
	CItemTextureStore m_itemTextures;

	// 把 m_arrNames 的内容刷新显示到输出框
	void RefreshNamesOutput(const CString& strSummary = CString());

	// 把子对话框生成好的 KubeJS 脚本显示到输出框（覆盖掉之前的内容）
	void ShowRecipeScript(const CString& strScript);

	afx_msg void OnBnClickedBtnclear();
	afx_msg void OnBnClickedBtndwnload();
	afx_msg void OnBnClickedBtnother();
	afx_msg void OnBnClickedBtnremove();
	afx_msg void OnBnClickedBtnmodify();
	CComboBox m_comboLanguage;
	afx_msg void OnCbnSelchangeCombolanguage();

private:
	template<class Dialog>
	void OpenRecipeDialog(Dialog& dialog)
	{
		dialog.SetItemSource(&m_arrNames);
		dialog.SetCatalog(&m_importResult.mapEntryTypes);
		dialog.SetTextureStore(&m_itemTextures);
		if (dialog.DoModal() == IDOK)
			ShowRecipeScript(dialog.GetRecipeScript());
	}
};
