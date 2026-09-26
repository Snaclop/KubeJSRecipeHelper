// CDlgSelect.h: 头文件
//

#pragma once
#include "afxdialogex.h"


// CDlgSelect 对话框：从已导入的物品列表里挑一个物品，并指定数量
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

	// 设置可选物品的来源列表。
	// 只保存指针，调用方需保证该列表的生命周期覆盖本对话框。
	void SetItemList(const CStringArray* pItemList);

	// 打开时预选的物品与数量，用于修改已经选过的格子
	void SetInitialSelection(const CString& strItem, int nCount);

	// 选择结果，DoModal 返回 IDOK 之后才有效
	CString GetSelectedItem() const { return m_strSelectedItem; }
	int     GetSelectedCount() const { return m_nSelectedCount; }

protected:
	// 数量的上下限，与 MC 单个格子的最大堆叠数一致
	enum { COUNT_MIN = 1, COUNT_MAX = 64 };

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	// 按搜索框的内容刷新物品列表；strPrefer 为刷新后希望保持选中的物品
	void RefreshList(const CString& strPrefer);
	// 取列表里当前选中的物品，返回 FALSE 表示没有选中真正的物品
	BOOL GetCurSelItem(CString& strItem) const;

	const CStringArray* m_pItemList;	// 可选物品的来源列表，可为空
	CString m_strInitialItem;			// 打开时预选的物品
	int     m_nInitialCount;			// 打开时预选的数量

	CString m_strSelectedItem;			// 结果：选中的物品 id
	int     m_nSelectedCount;			// 结果：选中的数量

	CListBox m_List;					// 物品列表
	CEdit    m_EditSearch;				// 搜索框
	CEdit    m_EditNumber;				// 数量
	CSpinButtonCtrl m_Spin;				// 数量微调按钮

	afx_msg void OnEnChangeEditSearch();
	afx_msg void OnLbnDblclkList();
	DECLARE_MESSAGE_MAP()
};
