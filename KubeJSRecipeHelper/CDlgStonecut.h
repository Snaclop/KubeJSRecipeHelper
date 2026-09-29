#pragma once
#include "afxdialogex.h"
#include "ItemTextureStore.h"

// 切石机配方对话框：输入格、输出格和说明文字均由代码绘制。
class CDlgStonecut : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgStonecut)

public:
	CDlgStonecut(CWnd* pParent = nullptr);
	virtual ~CDlgStonecut();
	void SetItemSource(const CStringArray* pItemSource);
	void SetTextureStore(CItemTextureStore* pTextureStore);
	CString GetRecipeScript() const { return m_strRecipeScript; }

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSTONECUT };
#endif

protected:
	enum { SLOT_INPUT, SLOT_OUTPUT, SLOT_COUNT };
	struct StonecutCell
	{
		CString strItem;
		int nCount = 0;
	};

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void CalcSlotRects();
	int HitTestSlot(CPoint point) const;
	void InvalidateSlot(int nSlot);
	void DrawSlots(CDC* pDC);
	void SelectSlotItem(int nSlot);
	CString BuildRecipeScript() const;

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	DECLARE_MESSAGE_MAP()

	const CStringArray* m_pItemSource = nullptr;
	CItemTextureStore* m_pTextureStore = nullptr;
	StonecutCell m_slots[SLOT_COUNT];
	CRect m_rcSlots[SLOT_COUNT];
	CRect m_rcLabels[SLOT_COUNT];
	CRect m_rcArrow;
	CRect m_rcHint;
	int m_nHoverSlot = -1;
	BOOL m_bTrackingMouse = FALSE;
	CString m_strRecipeScript;
};
