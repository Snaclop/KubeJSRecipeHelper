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
	m_ui.Initialize(this, 700, 380);

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
	m_rcSlots[SLOT_INPUT] = m_ui.Rect(150, 110, 70, 70);
	m_rcSlots[SLOT_OUTPUT] = m_ui.Rect(480, 110, 70, 70);
	m_rcArrow = m_ui.Rect(260, 110, 180, 70);
	m_rcLabels[SLOT_INPUT] = m_ui.Rect(150, 70, 70, 25);
	m_rcLabels[SLOT_OUTPUT] = m_ui.Rect(480, 70, 70, 25);
	m_rcHint = m_ui.Rect(22, 290, 656, 40);
	m_ui.Move(IDC_COMBOCOOK, 125, 16, 285, 160);
	m_ui.Move(IDC_EDITCOOKTIME, 135, 230, 100, 28);
	m_ui.Move(IDC_SPINCOOKTIME, 215, 230, 20, 28);
	m_SpinCookTime.SetBuddy(&m_EditCookTime);
	m_EditCookTime.ModifyStyle(0, ES_NUMBER);
	m_rcTypeLabel = m_ui.Rect(22, 16, 100, 29);
	m_rcTimeLabel = m_ui.Rect(22, 230, 110, 28);
	for (int slot = 0; slot < SLOT_COUNT; ++slot) InvalidateSlot(slot);
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
	if (nSlot < 0 || nSlot >= SLOT_COUNT) return;
	m_ui.Tip(nSlot + 1, m_rcSlots[nSlot], nSlot == SLOT_INPUT ? _T("原料") : _T("产物"),
		m_slots[nSlot].strItem, m_slots[nSlot].nCount);
	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgCook::OnPaint()
{
	CPaintDC dc(this);
	DrawSlots(&dc);
}

void CDlgCook::DrawSlots(CDC* pDC)
{
	m_ui.Text(pDC, _T("炉型"), m_rcTypeLabel);
	m_ui.Text(pDC, _T("处理时间"), m_rcTimeLabel);
	m_ui.Text(pDC, _T("tick（20 tick = 1 秒）"), m_ui.Rect(260, 230, 418, 28));
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

BOOL CDlgCook::PreTranslateMessage(MSG* message)
{
	m_ui.Relay(message);
	return CDialogEx::PreTranslateMessage(message);
}
