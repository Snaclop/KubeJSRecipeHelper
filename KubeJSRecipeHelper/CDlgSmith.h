#pragma once
#include "afxdialogex.h"


// CDlgSmith 对话框：锻造台
//
// 模板 / 升级 / 材料 / 输出四个灰色矩形对应资源里的四个静态控件，
// 左键点击任意一个矩形都会模态弹出 CDlgSelect 选择物品与数量，
// 选择结果直接显示在对应的矩形上。
class CDlgSmith : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSmith)

public:
	CDlgSmith(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgSmith();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSMITH };
#endif

	// 设置可选物品的来源列表（由主对话框传入）。
	// 只保存指针，调用方需保证该列表的生命周期覆盖本对话框。
	void SetItemSource(const CStringArray* pItemSource);

	// 取生成好的 KubeJS 脚本；四个槽位有任何一个为空时返回空串
	CString GetRecipeScript() const { return BuildRecipeScript(); }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnOK();

	// 四个矩形的槽位序号
	enum
	{
		SLOT_TEMPLATE = 0,	// 锻造模板
		SLOT_UPGRADE,		// 要升级的物品
		SLOT_INGREDIENT,	// 升级材料
		SLOT_OUTPUT,		// 输出
		SLOT_COUNT
	};

	// 单个槽位的内容
	struct SmithSlot
	{
		CString strItem;	// 物品 id，为空表示该槽位没有物品
		int     nCount;		// 物品数量

		SmithSlot() : nCount(0) {}
	};

	// 点击矩形后弹出选择对话框，把结果写到对应槽位与矩形上
	BOOL SelectSlotItem(int nSlot);
	// 把槽位内容刷新显示到对应的矩形控件上
	void UpdateSlotText(int nSlot);
	// 槽位序号换算成对应的静态控件 ID
	UINT GetSlotCtrlID(int nSlot) const;
	// 按四个槽位的内容生成 KubeJS 锻造台脚本（event.smithing）
	CString BuildRecipeScript() const;

	SmithSlot m_slots[SLOT_COUNT];		// 四个槽位的内容
	const CStringArray* m_pItemSource;	// 可选物品的来源列表，可为空
	CBrush m_brSlot;					// 矩形的灰色背景画刷

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnStnClickedStaticTemplate();
	afx_msg void OnStnClickedStaticUpgrade();
	afx_msg void OnStnClickedStaticIngredient();
	afx_msg void OnStnClickedStaticOutput();
	DECLARE_MESSAGE_MAP()
};
