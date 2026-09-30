#pragma once
#include "afxdialogex.h"
#include "ItemTextureStore.h"
#include "RecipeDialogUI.h"

// 熔炉、高炉和烟熏炉配方对话框
class CDlgCook : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgCook)

public:
	CDlgCook(CWnd* pParent = nullptr);
	virtual ~CDlgCook();
	void SetItemSource(const CStringArray* pItemSource);
	void SetTextureStore(CItemTextureStore* pTextureStore);
	CString GetRecipeScript() const { return m_strRecipeScript; }

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGCOOK };
#endif

protected:
	enum { SLOT_INPUT, SLOT_OUTPUT, SLOT_COUNT };
	struct CookCell
	{
		CString strItem;
		int nCount = 0;
	};

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* message) override;
	CRecipeDialogUI m_ui;
	virtual void OnOK();
	void CalcSlotRects();
	int HitTestSlot(CPoint point) const;
	void DrawSlots(CDC* pDC);
	void SelectSlotItem(int nSlot);
	CString BuildRecipeScript(int nType, int nTime) const;
	void InvalidateSlot(int nSlot);
	int GetCookType() const;

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnCbnSelchangeCombocook();
	DECLARE_MESSAGE_MAP()

	CComboBox m_ComboCookType;
	CEdit m_EditCookTime;
	CSpinButtonCtrl m_SpinCookTime;
	const CStringArray* m_pItemSource = nullptr;
	CItemTextureStore* m_pTextureStore = nullptr;
	CookCell m_slots[SLOT_COUNT];
	CRect m_rcSlots[SLOT_COUNT];
	CRect m_rcArrow;
	CRect m_rcLabels[SLOT_COUNT];
	CRect m_rcTypeLabel;
	CRect m_rcTimeLabel;
	CRect m_rcHint;
	int m_nHoverSlot = -1;
	BOOL m_bTrackingMouse = FALSE;
	CString m_strRecipeScript;
};
