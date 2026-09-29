// CDlgStonecut.cpp: 切石机配方

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgStonecut.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	const COLORREF CR_SLOT = RGB(0x8B, 0x8B, 0x8B);
	const COLORREF CR_HOVER = RGB(0xB0, 0xB0, 0xB0);
	const COLORREF CR_DARK = RGB(0x37, 0x37, 0x37);
	const COLORREF CR_LIGHT = RGB(0xFF, 0xFF, 0xFF);
}

IMPLEMENT_DYNAMIC(CDlgStonecut, CDialogEx)

CDlgStonecut::CDlgStonecut(CWnd* pParent)
	: CDialogEx(IDD_DLGSTONECUT, pParent)
{
}

CDlgStonecut::~CDlgStonecut()
{
}

void CDlgStonecut::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}

void CDlgStonecut::SetTextureStore(CItemTextureStore* pTextureStore)
{
	m_pTextureStore = pTextureStore;
}

void CDlgStonecut::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDlgStonecut, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

BOOL CDlgStonecut::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CalcSlotRects();
	return TRUE;
}

void CDlgStonecut::CalcSlotRects()
{
	CRect rcClient, rcButton;
	GetClientRect(&rcClient);
	GetDlgItem(IDOK)->GetWindowRect(&rcButton);
	ScreenToClient(&rcButton);

	int nCell = rcClient.Width() / 5;
	if (nCell > 112)
		nCell = 112;
	const int nAvailableH = rcButton.top - 90;
	if (nCell > nAvailableH / 2)
		nCell = nAvailableH / 2;
	if (nCell < 24)
		nCell = 24;
	const int nTop = max(35, (rcButton.top - nCell) / 2 - 35);
	const int nInputX = rcClient.Width() / 4 - nCell / 2;
	const int nOutputX = rcClient.Width() * 3 / 4 - nCell / 2;
	m_rcSlots[SLOT_INPUT].SetRect(nInputX, nTop, nInputX + nCell, nTop + nCell);
	m_rcSlots[SLOT_OUTPUT].SetRect(nOutputX, nTop, nOutputX + nCell, nTop + nCell);
	m_rcArrow.SetRect(m_rcSlots[SLOT_INPUT].right + 12, nTop,
		m_rcSlots[SLOT_OUTPUT].left - 12, nTop + nCell);
	for (int i = 0; i < SLOT_COUNT; ++i)
		m_rcLabels[i].SetRect(m_rcSlots[i].left - 18, m_rcSlots[i].bottom + 4,
			m_rcSlots[i].right + 18, m_rcSlots[i].bottom + 28);
	m_rcHint.SetRect(20, rcButton.top - 78, rcClient.right - 20, rcButton.top - 40);
}

int CDlgStonecut::HitTestSlot(CPoint point) const
{
	for (int i = 0; i < SLOT_COUNT; ++i)
		if (m_rcSlots[i].PtInRect(point))
			return i;
	return -1;
}

void CDlgStonecut::InvalidateSlot(int nSlot)
{
	if (nSlot >= 0 && nSlot < SLOT_COUNT)
		InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgStonecut::OnPaint()
{
	CDialogEx::OnPaint();
	CClientDC dc(this);
	DrawSlots(&dc);
}

void CDlgStonecut::DrawSlots(CDC* pDC)
{
	CFont* pFont = GetFont();
	CFont* pOldFont = pFont != nullptr ? pDC->SelectObject(pFont) : nullptr;
	pDC->SetBkMode(TRANSPARENT);

	if (m_rcArrow.Width() >= 8)
	{
		const int y = m_rcArrow.CenterPoint().y;
		const int x = m_rcArrow.left + m_rcArrow.Width() * 2 / 3;
		const int nHalfHead = max(4, m_rcArrow.Height() / 7);
		POINT arrow[] =
		{
			{ x, y - nHalfHead }, { m_rcArrow.right, y }, { x, y + nHalfHead }
		};
		CPen pen(PS_SOLID, 2, CR_DARK);
		CBrush brush(CR_DARK);
		CPen* pOldPen = pDC->SelectObject(&pen);
		CBrush* pOldBrush = pDC->SelectObject(&brush);
		pDC->MoveTo(m_rcArrow.left, y);
		pDC->LineTo(x, y);
		pDC->Polygon(arrow, _countof(arrow));
		pDC->SelectObject(pOldBrush);
		pDC->SelectObject(pOldPen);
	}

	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		const CRect& rc = m_rcSlots[i];
		pDC->FillSolidRect(rc, i == m_nHoverSlot ? CR_HOVER : CR_SLOT);
		pDC->FillSolidRect(rc.left, rc.top, rc.Width() - 1, 1, CR_DARK);
		pDC->FillSolidRect(rc.left, rc.top, 1, rc.Height() - 1, CR_DARK);
		pDC->FillSolidRect(rc.left + 1, rc.bottom - 1, rc.Width() - 1, 1, CR_LIGHT);
		pDC->FillSolidRect(rc.right - 1, rc.top + 1, 1, rc.Height() - 1, CR_LIGHT);

		const StonecutCell& cell = m_slots[i];
		if (cell.strItem.IsEmpty())
			continue;
		if (m_pTextureStore == nullptr || !m_pTextureStore->Draw(pDC, rc, cell.strItem))
		{
			CRect rcText(rc);
			rcText.DeflateRect(3, 3);
			pDC->SetTextColor(RGB(0x20, 0x20, 0x20));
			pDC->DrawText(cell.strItem, rcText,
				DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
		}
		if (cell.nCount > 1)
		{
			CString strCount;
			strCount.Format(_T("%d"), cell.nCount);
			CRect rcCount(rc.right - 35, rc.bottom - 22, rc.right - 3, rc.bottom - 2);
			CRect rcShadow(rcCount);
			rcShadow.OffsetRect(1, 1);
			pDC->SetTextColor(CR_DARK);
			pDC->DrawText(strCount, rcShadow, DT_RIGHT | DT_SINGLELINE);
			pDC->SetTextColor(CR_LIGHT);
			pDC->DrawText(strCount, rcCount, DT_RIGHT | DT_SINGLELINE);
		}
	}

	pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	pDC->DrawText(_T("原料"), m_rcLabels[SLOT_INPUT], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC->DrawText(_T("产物"), m_rcLabels[SLOT_OUTPUT], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
	pDC->DrawText(_T("左键选择物品，右键清空格子"), m_rcHint,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	if (pOldFont != nullptr)
		pDC->SelectObject(pOldFont);
}

void CDlgStonecut::SelectSlotItem(int nSlot)
{
	if (m_pItemSource == nullptr || m_pItemSource->GetSize() == 0)
	{
		AfxMessageBox(_T("还没有可选的物品，请先在主界面点击“导入 jar 文件”。"), MB_ICONINFORMATION);
		return;
	}
	CDlgSelect dlgSelect;
	dlgSelect.SetItemList(m_pItemSource);
	dlgSelect.SetCountLimit(nSlot == SLOT_INPUT ? 1 : 64);
	if (!m_slots[nSlot].strItem.IsEmpty())
		dlgSelect.SetInitialSelection(m_slots[nSlot].strItem, m_slots[nSlot].nCount);
	if (dlgSelect.DoModal() == IDOK)
	{
		m_slots[nSlot].strItem = dlgSelect.GetSelectedItem();
		m_slots[nSlot].nCount = dlgSelect.GetSelectedCount();
		InvalidateSlot(nSlot);
	}
}

void CDlgStonecut::OnLButtonDown(UINT nFlags, CPoint point)
{
	const int nSlot = HitTestSlot(point);
	if (nSlot >= 0)
		SelectSlotItem(nSlot);
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgStonecut::OnRButtonDown(UINT nFlags, CPoint point)
{
	const int nSlot = HitTestSlot(point);
	if (nSlot >= 0)
	{
		m_slots[nSlot].strItem.Empty();
		m_slots[nSlot].nCount = 0;
		InvalidateSlot(nSlot);
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}

void CDlgStonecut::OnMouseMove(UINT nFlags, CPoint point)
{
	const int nSlot = HitTestSlot(point);
	if (nSlot != m_nHoverSlot)
	{
		InvalidateSlot(m_nHoverSlot);
		m_nHoverSlot = nSlot;
		InvalidateSlot(m_nHoverSlot);
	}
	if (!m_bTrackingMouse)
	{
		TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = GetSafeHwnd();
		m_bTrackingMouse = _TrackMouseEvent(&tme);
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CDlgStonecut::OnMouseLeave()
{
	m_bTrackingMouse = FALSE;
	InvalidateSlot(m_nHoverSlot);
	m_nHoverSlot = -1;
	CDialogEx::OnMouseLeave();
}

void CDlgStonecut::OnOK()
{
	if (m_slots[SLOT_INPUT].strItem.IsEmpty() || m_slots[SLOT_OUTPUT].strItem.IsEmpty())
	{
		AfxMessageBox(_T("请先选择切石原料和产物。"), MB_ICONINFORMATION);
		return;
	}
	m_strRecipeScript = BuildRecipeScript();
	CDialogEx::OnOK();
}

CString CDlgStonecut::BuildRecipeScript() const
{
	CString strOutput;
	const StonecutCell& output = m_slots[SLOT_OUTPUT];
	if (output.nCount > 1)
		strOutput.Format(_T("Item.of('%s', %d)"), output.strItem.GetString(), output.nCount);
	else
		strOutput.Format(_T("'%s'"), output.strItem.GetString());
	CString strScript;
	strScript.Format(
		_T("ServerEvents.recipes(event => {\r\n")
		_T("  // 切石机 %s\r\n")
		_T("  event.stonecutting(%s, '%s')\r\n")
		_T("})\r\n"),
		output.strItem.GetString(), strOutput.GetString(),
		m_slots[SLOT_INPUT].strItem.GetString());
	return strScript;
}
