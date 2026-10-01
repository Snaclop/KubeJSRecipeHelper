#pragma once
#include "afxdialogex.h"
#include "ItemTextureStore.h"
#include "RecipeDialogUI.h"

// 工作台共用九宫格，通过单选框选择有序或无序合成。
class CDlgCraftable : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgCraftable)
public:
	CDlgCraftable(CWnd* pParent = nullptr);
	~CDlgCraftable() override;
	void SetItemSource(const CStringArray* pItemSource);
	void SetTextureStore(CItemTextureStore* pTextureStore);
	CString GetRecipeScript() const { return BuildRecipeScript(); }
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGCRAFTABLE };
#endif
protected:
	enum { GRID_ROWS = 3, GRID_COLS = 3, GRID_CELLS = 9,
		SLOT_OUTPUT = GRID_CELLS, SLOT_COUNT = GRID_CELLS + 1 };
	struct CraftCell
	{
		CString strItem;
		int nCount = 0;
		BOOL IsEmpty() const { return strItem.IsEmpty(); }
	};
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	BOOL PreTranslateMessage(MSG* message) override;
	void OnOK() override;
	CString BuildRecipeScript() const;
	CString BuildShapedScript() const;
	CString BuildShapelessScript() const;
	LPCTSTR GetHintText() const;
	BOOL ValidateSlots(CString& strError) const;
	void CalcSlotRects();
	CraftCell* GetSlot(int nSlot);
	const CraftCell* GetSlot(int nSlot) const;
	BOOL HitTestSlot(CPoint point, int& nSlot) const;
	void InvalidateSlot(int nSlot);
	void DrawSlots(CDC* pDC);
	BOOL SelectSlotItem(int nSlot);

	CRecipeDialogUI m_ui;
	CraftCell m_cells[GRID_ROWS][GRID_COLS];
	CraftCell m_outputCell;
	CRect m_rcSlots[SLOT_COUNT];
	CRect m_rcHint;
	int m_nHoverSlot;
	BOOL m_bTrackingMouse;
	bool m_bShapeless = false;
	const CStringArray* m_pItemSource;
	CItemTextureStore* m_pTextureStore;

	afx_msg void OnRecipeTypeChanged();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	DECLARE_MESSAGE_MAP()
};
