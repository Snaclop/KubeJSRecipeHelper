// CDlgSelect.h: 头文件
//

#pragma once
#include "afxdialogex.h"
#include "RecipeCatalog.h"
#include <vector>


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
	// 某些配方只允许单个原料，或限制产物堆叠数量。
	void SetCountLimit(int nMaxCount);
	void SetCatalog(const RecipeTypeMap* catalog, unsigned allowedKinds) { m_catalog = catalog; m_allowedKinds = allowedKinds; }
	void SetChanceMode(bool enabled, bool weights = false) { m_allowChance = enabled; m_weights = weights; m_hasChanceMode = true; }
	void SetInitialDetails(RecipeEntryKind kind, double chance) { m_kind = kind; m_chance = chance; }
	RecipeEntryKind GetSelectedKind() const { return m_kind; }
	double GetSelectedChance() const { return m_chance; }

	// 选择结果，DoModal 返回 IDOK 之后才有效
	CString GetSelectedItem() const { return m_strSelectedItem; }
	int     GetSelectedCount() const { return m_nSelectedCount; }

protected:
	// 数量的上下限，考虑液体兼容，最大值为 99999999
	enum { COUNT_MIN = 1, COUNT_MAX = 99999999 };

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
	int     m_nMaxCount;				// 当前选择器允许的最大数量

	CListBox m_List;					// 物品列表
	CEdit    m_EditSearch;				// 搜索框
	CEdit    m_EditNumber;				// 数量
	CSpinButtonCtrl m_SpinNumber;		// 数量微调按钮

	afx_msg void OnEnChangeEditSearch();
	afx_msg void OnLbnDblclkList();
	afx_msg void OnSelectionChanged();
	afx_msg void OnChanceSpin(NMHDR* header, LRESULT* result);
	const RecipeTypeMap* m_catalog = nullptr;
	unsigned m_allowedKinds = RecipeItem;
	RecipeEntryKind m_kind = RecipeItem;
	double m_chance = 100;
	bool m_allowChance = true, m_weights = false;
	bool m_hasChanceMode = false;
	struct Choice { CString id; RecipeEntryKind kind; };
	std::vector<Choice> m_choices;
	int CurrentLimit() const;
	DECLARE_MESSAGE_MAP()
	CSpinButtonCtrl m_SpinPossibility;
	CEdit m_EditPossibility;
};
