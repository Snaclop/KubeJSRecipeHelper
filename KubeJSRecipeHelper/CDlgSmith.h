#pragma once
#include "afxdialogex.h"


// CDlgSmith 对话框：锻造台
//
// 模板 / 升级物品 / 原材料 / 输出四个格子与有序、无序合成一样，完全由代码在
// 对话框客户区上直接绘制，不依赖 .rc 里的控件：
//   - 左键点击某个格子，模态弹出 CDlgSelect 选择物品与数量；
//   - 右键点击某个格子，清空该格；
//   - 鼠标悬停时格子底色变亮，配色跟合成界面完全一致。
//
// 三个输入格摆在“输入”分组框里，输出格放在分组框右边，中间画一个箭头；
// 点“确定”后按四个格子生成 KubeJS 锻造台脚本（event.smithing）。
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

	// 取生成好的 KubeJS 脚本；四个格子有任何一个为空时返回空串
	CString GetRecipeScript() const { return BuildRecipeScript(); }

protected:
	// 四个格子的序号（前三个是输入，跟 MC 锻造台的摆法一致）
	enum
	{
		SLOT_TEMPLATE = 0,	// 锻造模板
		SLOT_UPGRADE,		// 要升级的物品
		SLOT_INGREDIENT,	// 升级材料
		SLOT_OUTPUT,		// 输出
		SLOT_COUNT
	};

	// 单个格子的内容
	struct SmithCell
	{
		CString strItem;	// 物品 id（形如 minecraft:netherite_ingot），为空表示该格没有物品
		int     nCount;		// 物品数量

		SmithCell() : nCount(0) {}
		BOOL IsEmpty() const { return strItem.IsEmpty(); }
	};

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	// 按“输入”分组框算出四个格子与箭头的位置，并把说明文字对齐到格子下面
	void CalcSlotRects();

	SmithCell* GetSlot(int nSlot);							// 取某个格子里的内容
	const SmithCell* GetSlot(int nSlot) const;
	BOOL HitTestSlot(CPoint point, int& nSlot) const;		// 客户区坐标换算成格子序号
	void InvalidateSlot(int nSlot);							// 只重画一个格子

	// 点击格子后弹出选择对话框，返回 TRUE 表示用户确认了选择
	BOOL SelectSlotItem(int nSlot);

	// 绘制
	void DrawSlots(CDC* pDC);
	void DrawSlotFrame(CDC* pDC, const CRect& rect, BOOL bHover);
	void DrawSlotItem(CDC* pDC, const CRect& rect, const CString& strItem, int nCount);
	void DrawArrow(CDC* pDC);								// 输入格与输出格之间的箭头

	// 按四个格子的内容生成 KubeJS 锻造台脚本（event.smithing）
	CString BuildRecipeScript() const;

	SmithCell m_slots[SLOT_COUNT];		// 四个格子的内容
	CRect m_rcSlots[SLOT_COUNT];		// 每个格子的矩形
	CRect m_rcArrow;					// 箭头占用的矩形
	int   m_nCellSize;					// 单个格子的边长
	int   m_nHoverSlot;					// 鼠标悬停的格子序号，-1 表示没有悬停
	BOOL  m_bTrackingMouse;				// 是否已经请求过 WM_MOUSELEAVE
	CFont m_fontSmall;					// 画命名空间、数量用的小号字体

	const CStringArray* m_pItemSource;	// 可选物品的来源列表，可为空

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	DECLARE_MESSAGE_MAP()
};