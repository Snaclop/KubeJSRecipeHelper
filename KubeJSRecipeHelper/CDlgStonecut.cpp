// CDlgStonecut.cpp: 切石机配方

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgStonecut.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
	m_ui.Initialize(this, 700, 380);
	CalcSlotRects();
	return TRUE;
}

void CDlgStonecut::CalcSlotRects()
{
	m_rcSlots[SLOT_INPUT] = m_ui.Rect(150, 110, 70, 70);
	m_rcSlots[SLOT_OUTPUT] = m_ui.Rect(480, 110, 70, 70);
	m_rcArrow = m_ui.Rect(260, 110, 180, 70);
	m_rcLabels[SLOT_INPUT] = m_ui.Rect(150, 70, 70, 25);
	m_rcLabels[SLOT_OUTPUT] = m_ui.Rect(480, 70, 70, 25);
	m_rcHint = m_ui.Rect(22, 290, 656, 40);
	for (int slot = 0; slot < SLOT_COUNT; ++slot) InvalidateSlot(slot);
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
	if (nSlot < 0 || nSlot >= SLOT_COUNT) return;
	m_ui.Tip(nSlot + 1, m_rcSlots[nSlot], nSlot == SLOT_INPUT ? _T("原料") : _T("产物"),
		m_slots[nSlot].strItem, m_slots[nSlot].nCount);
	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgStonecut::OnPaint()
{
	CPaintDC dc(this);
	DrawSlots(&dc);
}

void CDlgStonecut::DrawSlots(CDC* pDC)
{
	m_ui.Text(pDC, _T("切石机"), m_ui.Rect(22, 16, 656, 29));
	m_ui.Text(pDC, _T("原料"), m_rcLabels[SLOT_INPUT], false, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	m_ui.Text(pDC, _T("产物"), m_rcLabels[SLOT_OUTPUT], false, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	m_ui.Arrow(pDC, m_rcArrow);
	for (int slot = 0; slot < SLOT_COUNT; ++slot)
	{
		m_ui.Slot(pDC, m_rcSlots[slot], slot == m_nHoverSlot, m_slots[slot].strItem, m_slots[slot].nCount, m_pTextureStore);
		m_ui.Text(pDC, _T("物品"), m_ui.Rect(slot == SLOT_INPUT ? 150 : 480, 184, 70, 27), false, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	}
	m_ui.Text(pDC, _T("左键选择，右键清空；停留在格子上可查看 ID 和数量。"), m_rcHint, true, DT_LEFT | DT_WORDBREAK);
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

BOOL CDlgStonecut::PreTranslateMessage(MSG* message)
{
	m_ui.Relay(message);
	return CDialogEx::PreTranslateMessage(message);
}
