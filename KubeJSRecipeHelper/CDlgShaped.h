// CDlgShaped.h: 头文件
//

#pragma once
#include "afxdialogex.h"


// CDlgShaped 对话框：有序合成（工作台的九宫格 + 输出格）
//
// 九宫格和输出格都由代码在对话框客户区上直接绘制，不依赖 .rc 里的任何控件：
//   - 左键点击某个格子，模态弹出 CDlgSelect 选择物品与数量；
//   - 右键点击某个格子，清空该格；
//   - 点“确定”后按九宫格的内容生成 KubeJS 有序合成脚本。
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

	// 设置可选物品的来源列表（由主对话框传入）。
	// 只保存指针，调用方需保证该列表的生命周期覆盖本对话框。
	void SetItemSource(const CStringArray* pItemSource);

	// 取生成好的 KubeJS 有序合成脚本；九宫格或产物为空时返回空串
	CString GetRecipeScript() const;

protected:
	// 格子序号：0 ~ 8 是九宫格的九个合成格，9 是右边的输出格
	enum
	{
		GRID_ROWS = 3,
		GRID_COLS = 3,
		GRID_CELLS = GRID_ROWS * GRID_COLS,
		SLOT_OUTPUT = GRID_CELLS,
		SLOT_COUNT = GRID_CELLS + 1
	};

	// 单个格子的内容
	struct CraftCell
	{
		CString strItem;	// 物品 id（形如 minecraft:oak_planks），为空表示该格没有物品
		int     nCount;		// 物品数量

		CraftCell() : nCount(0) {}
		BOOL IsEmpty() const { return strItem.IsEmpty(); }
	};

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	// 格子的位置计算
	void CalcSlotRects();										// 按客户区算出格子大小与摆放位置
	CraftCell* GetSlot(int nSlot);								// 取某个格子里的内容
	const CraftCell* GetSlot(int nSlot) const;
	BOOL HitTestSlot(CPoint point, int& nSlot) const;			// 客户区坐标换算成格子序号
	void InvalidateSlot(int nSlot);								// 只重画一个格子

	// 绘制
	void DrawSlots(CDC* pDC);
	void DrawSlotFrame(CDC* pDC, const CRect& rect, BOOL bHover);
	void DrawSlotItem(CDC* pDC, const CRect& rect, const CString& strItem, int nCount);
	void DrawArrow(CDC* pDC);									// 九宫格与输出格之间的箭头

	// 点击格子后弹出选择对话框，返回 TRUE 表示用户确认了选择
	BOOL SelectSlotItem(int nSlot);

	CraftCell m_cells[GRID_ROWS][GRID_COLS];	// 九个合成格的内容
	CraftCell m_outputCell;						// 输出格的内容
	CRect m_rcSlots[SLOT_COUNT];				// 每个格子的矩形
	CRect m_rcSlotArea;							// 所有格子（含中间箭头）占用的整块区域
	CRect m_rcHint;								// 操作提示文字占用的矩形
	int   m_nCellSize;							// 单个格子的边长
	int   m_nHoverSlot;							// 鼠标悬停的格子序号，-1 表示没有悬停
	BOOL  m_bTrackingMouse;						// 是否已经请求过 WM_MOUSELEAVE
	CFont m_fontSmall;							// 画命名空间、数量用的小号字体

	const CStringArray* m_pItemSource;			// 可选物品的来源列表，可为空

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	DECLARE_MESSAGE_MAP()
};
