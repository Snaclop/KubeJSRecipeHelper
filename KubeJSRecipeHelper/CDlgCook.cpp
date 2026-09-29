// CDlgCook.cpp: 熔炉、高炉和烟熏炉配方

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgCook.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	enum { COOK_FURNACE, COOK_BLAST, COOK_SMOKER };
	const int COOK_TIME_MAX = 1000000;
	const COLORREF CR_SLOT = RGB(0x8B, 0x8B, 0x8B);
	const COLORREF CR_HOVER = RGB(0xB0, 0xB0, 0xB0);
	const COLORREF CR_DARK = RGB(0x37, 0x37, 0x37);
	const COLORREF CR_LIGHT = RGB(0xFF, 0xFF, 0xFF);
}

IMPLEMENT_DYNAMIC(CDlgCook, CDialogEx)

CDlgCook::CDlgCook(CWnd* pParent)
	: CDialogEx(IDD_DLGCOOK, pParent)
{
}

CDlgCook::~CDlgCook()
{
}

void CDlgCook::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}

void CDlgCook::SetTextureStore(CItemTextureStore* pTextureStore)
{
	m_pTextureStore = pTextureStore;
}

void CDlgCook::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOCOOK, m_ComboCookType);
	DDX_Control(pDX, IDC_EDITCOOKTIME, m_EditCookTime);
	DDX_Control(pDX, IDC_SPINCOOKTIME, m_SpinCookTime);
}

BEGIN_MESSAGE_MAP(CDlgCook, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_CBN_SELCHANGE(IDC_COMBOCOOK, &CDlgCook::OnCbnSelchangeCombocook)
END_MESSAGE_MAP()

BOOL CDlgCook::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// CBS_SORT 会重排条目，因此用 item data 保存类型。
	const struct { LPCTSTR name; int type; } cookTypes[] =
	{
		{ _T("熔炉"), COOK_FURNACE },
		{ _T("高炉"), COOK_BLAST },
		{ _T("烟熏炉"), COOK_SMOKER }
	};
	for (const auto& cookType : cookTypes)
	{
		const int nIndex = m_ComboCookType.AddString(cookType.name);
		if (nIndex != CB_ERR && nIndex != CB_ERRSPACE)
			m_ComboCookType.SetItemData(nIndex, cookType.type);
	}
	m_ComboCookType.SelectString(-1, _T("熔炉"));
	m_SpinCookTime.SetRange32(1, COOK_TIME_MAX);
	m_SpinCookTime.SetPos32(200);
	SetDlgItemInt(IDC_EDITCOOKTIME, 200, FALSE);
	CalcSlotRects();
	return TRUE;
}

void CDlgCook::CalcSlotRects()
{
	CRect rcClient, rcCombo, rcTime, rcButton;
	GetClientRect(&rcClient);
	m_ComboCookType.GetWindowRect(&rcCombo);
	ScreenToClient(&rcCombo);
	m_EditCookTime.GetWindowRect(&rcTime);
	ScreenToClient(&rcTime);
	GetDlgItem(IDOK)->GetWindowRect(&rcButton);
	ScreenToClient(&rcButton);

	// 两个物品格位于炉型下方、处理时间上方。
	const int nTop = rcCombo.top + m_ComboCookType.GetItemHeight(-1) + 16;
	int nCell = rcTime.top - nTop - 26;
	if (nCell > 112)
		nCell = 112;
	if (nCell < 24)
		nCell = 24;
	const int nInputX = rcClient.Width() / 4 - nCell / 2;
	const int nOutputX = rcClient.Width() * 3 / 4 - nCell / 2;
	m_rcSlots[SLOT_INPUT].SetRect(nInputX, nTop, nInputX + nCell, nTop + nCell);
	m_rcSlots[SLOT_OUTPUT].SetRect(nOutputX, nTop, nOutputX + nCell, nTop + nCell);
	m_rcArrow.SetRect(m_rcSlots[SLOT_INPUT].right + 12, nTop,
		m_rcSlots[SLOT_OUTPUT].left - 12, nTop + nCell);
	for (int i = 0; i < SLOT_COUNT; ++i)
		m_rcLabels[i].SetRect(m_rcSlots[i].left - 18, m_rcSlots[i].bottom + 2,
			m_rcSlots[i].right + 18, m_rcSlots[i].bottom + 24);
	m_rcTypeLabel.SetRect(rcCombo.left, rcCombo.top - 25, rcCombo.left + 120, rcCombo.top - 2);
	m_rcTimeLabel.SetRect(rcTime.left - 120, rcTime.top - 29,
		rcTime.right + 25, rcTime.top - 3);
	m_rcHint.SetRect(20, rcTime.bottom + 20, rcClient.right - 20, rcButton.top - 4);
}

int CDlgCook::HitTestSlot(CPoint point) const
{
	for (int i = 0; i < SLOT_COUNT; ++i)
		if (m_rcSlots[i].PtInRect(point))
			return i;
	return -1;
}

void CDlgCook::InvalidateSlot(int nSlot)
{
	if (nSlot >= 0 && nSlot < SLOT_COUNT)
		InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgCook::OnPaint()
{
	CDialogEx::OnPaint();
	CClientDC dc(this);
	DrawSlots(&dc);
}

void CDlgCook::DrawSlots(CDC* pDC)
{
	CFont* pFont = GetFont();
	CFont* pOldFont = pFont != nullptr ? pDC->SelectObject(pFont) : nullptr;
	pDC->SetBkMode(TRANSPARENT);

	// 箭头和格子保持与合成、锻造窗口相同的配色。
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

		const CookCell& cell = m_slots[i];
		if (cell.strItem.IsEmpty())
			continue;
		if (m_pTextureStore == nullptr || !m_pTextureStore->Draw(pDC, rc, cell.strItem))
		{
			CRect rcText(rc);
			rcText.DeflateRect(3, 3);
			pDC->SetTextColor(RGB(0x20, 0x20, 0x20));
			pDC->DrawText(cell.strItem, rcText, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
		}
		if (cell.nCount > 1)
		{
			CString strCount;
			strCount.Format(_T("%d"), cell.nCount);
			CRect rcCount(rc.right - 35, rc.bottom - 22, rc.right - 3, rc.bottom - 2);
			pDC->SetTextColor(CR_DARK);
			CRect rcShadow(rcCount);
			rcShadow.OffsetRect(1, 1);
			pDC->DrawText(strCount, rcShadow, DT_RIGHT | DT_SINGLELINE);
			pDC->SetTextColor(CR_LIGHT);
			pDC->DrawText(strCount, rcCount, DT_RIGHT | DT_SINGLELINE);
		}
	}

	pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	pDC->DrawText(_T("炉型"), m_rcTypeLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	pDC->DrawText(_T("处理时间（tick）"), m_rcTimeLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	pDC->DrawText(_T("原料"), m_rcLabels[SLOT_INPUT], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC->DrawText(_T("产物"), m_rcLabels[SLOT_OUTPUT], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
	pDC->DrawText(_T("左键选择物品，右键清空格子"), m_rcHint,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	if (pOldFont != nullptr)
		pDC->SelectObject(pOldFont);
}

void CDlgCook::SelectSlotItem(int nSlot)
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

void CDlgCook::OnLButtonDown(UINT nFlags, CPoint point)
{
	const int nSlot = HitTestSlot(point);
	if (nSlot >= 0)
		SelectSlotItem(nSlot);
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgCook::OnRButtonDown(UINT nFlags, CPoint point)
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

void CDlgCook::OnMouseMove(UINT nFlags, CPoint point)
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

void CDlgCook::OnMouseLeave()
{
	m_bTrackingMouse = FALSE;
	InvalidateSlot(m_nHoverSlot);
	m_nHoverSlot = -1;
	CDialogEx::OnMouseLeave();
}

int CDlgCook::GetCookType() const
{
	const int nIndex = m_ComboCookType.GetCurSel();
	return nIndex == CB_ERR ? -1 : static_cast<int>(m_ComboCookType.GetItemData(nIndex));
}

void CDlgCook::OnCbnSelchangeCombocook()
{
	const int nDefaultTime = GetCookType() == COOK_FURNACE ? 200 : 100;
	m_SpinCookTime.SetPos32(nDefaultTime);
	SetDlgItemInt(IDC_EDITCOOKTIME, nDefaultTime, FALSE);
}

void CDlgCook::OnOK()
{
	if (GetCookType() < 0 || m_slots[SLOT_INPUT].strItem.IsEmpty() ||
		m_slots[SLOT_OUTPUT].strItem.IsEmpty())
	{
		AfxMessageBox(_T("请选择炉型、原料和产物。"), MB_ICONINFORMATION);
		return;
	}
	BOOL bValid = FALSE;
	const UINT nTime = GetDlgItemInt(IDC_EDITCOOKTIME, &bValid, FALSE);
	if (!bValid || nTime < 1 || nTime > COOK_TIME_MAX)
	{
		AfxMessageBox(_T("处理时间请输入 1 到 1000000 之间的整数 tick。"), MB_ICONINFORMATION);
		m_EditCookTime.SetFocus();
		m_EditCookTime.SetSel(0, -1);
		return;
	}
	m_strRecipeScript = BuildRecipeScript(GetCookType(), static_cast<int>(nTime));
	CDialogEx::OnOK();
}

CString CDlgCook::BuildRecipeScript(int nType, int nTime) const
{
	LPCTSTR pszMethod = _T("smelting");
	LPCTSTR pszLabel = _T("熔炉");
	if (nType == COOK_BLAST)
	{
		pszMethod = _T("blasting");
		pszLabel = _T("高炉");
	}
	else if (nType == COOK_SMOKER)
	{
		pszMethod = _T("smoking");
		pszLabel = _T("烟熏炉");
	}
	CString strOutput;
	const CookCell& output = m_slots[SLOT_OUTPUT];
	if (output.nCount > 1)
		strOutput.Format(_T("Item.of('%s', %d)"), output.strItem.GetString(), output.nCount);
	else
		strOutput.Format(_T("'%s'"), output.strItem.GetString());
	CString strScript;
	strScript.Format(
		_T("ServerEvents.recipes(event => {\r\n")
		_T("  // %s %s\r\n")
		_T("  event.%s(%s, '%s').cookingTime(%d)\r\n")
		_T("})\r\n"),
		pszLabel, output.strItem.GetString(), pszMethod, strOutput.GetString(),
		m_slots[SLOT_INPUT].strItem.GetString(), nTime);
	return strScript;
}
