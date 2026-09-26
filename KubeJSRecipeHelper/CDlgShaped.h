// CDlgShaped.h: 头文件
//

#pragma once
#include "afxdialogex.h"


// CDlgShaped 对话框：有序合成（工作台的九宫格）
//
// 九宫格完全由代码在对话框客户区上直接绘制，不依赖 .rc 里的任何控件：
//   - 左键点击某个方格，模态弹出 CDlgSelect 选择物品与数量；
//   - 右键点击某个方格，清空该格。
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

protected:
	// 九宫格的规模
	enum { GRID_ROWS = 3, GRID_COLS = 3, GRID_CELLS = GRID_ROWS * GRID_COLS };

	// 单个合成格的内容
	struct CraftCell
	{
		CString strItem;	// 物品 id（形如 minecraft:oak_planks），为空表示该格没有物品
		int     nCount;		// 物品数量

		CraftCell() : nCount(0) {}
		BOOL IsEmpty() const { return strItem.IsEmpty(); }
	};

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	// 九宫格的位置计算
	void CalcGridRect();										// 按客户区算出格子大小与摆放位置
	void GetCellRect(int nRow, int nCol, CRect& rect) const;	// 取某个方格的矩形
	BOOL HitTestCell(CPoint point, int& nRow, int& nCol) const;	// 客户区坐标换算成行列
	void InvalidateCell(int nRow, int nCol);					// 只重画一个方格

	// 绘制
	void DrawGrid(CDC* pDC);
	void DrawCell(CDC* pDC, int nRow, int nCol, const CRect& rect, BOOL bHover);
	void DrawCellItem(CDC* pDC, const CRect& rect, const CString& strItem, int nCount);

	// 点击方格后弹出选择对话框，返回 TRUE 表示用户确认了选择
	BOOL SelectCellItem(int nRow, int nCol);

	CraftCell m_cells[GRID_ROWS][GRID_COLS];	// 九个方格的内容
	CRect m_rcGrid;			// 九宫格整体占用的矩形
	CRect m_rcHint;			// 操作提示文字占用的矩形
	int   m_nCellSize;		// 单个方格的边长
	int   m_nHoverRow;		// 鼠标悬停的方格行，-1 表示没有悬停
	int   m_nHoverCol;
	BOOL  m_bTrackingMouse;	// 是否已经请求过 WM_MOUSELEAVE
	CFont m_fontSmall;		// 画命名空间、数量用的小号字体

	const CStringArray* m_pItemSource;	// 可选物品的来源列表，可为空

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	DECLARE_MESSAGE_MAP()
};
