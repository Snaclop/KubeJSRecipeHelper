// CDlgSmith.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgSmith.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDlgSmith 对话框

IMPLEMENT_DYNAMIC(CDlgSmith, CDialogEx)

CDlgSmith::CDlgSmith(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGSMITH, pParent)
	, m_nHoverSlot(-1)
	, m_bTrackingMouse(FALSE)
	, m_pItemSource(nullptr)
	, m_pTextureStore(nullptr)
{
}

CDlgSmith::~CDlgSmith()
{
}

void CDlgSmith::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CDlgSmith::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_ui.Initialize(this, 700, 360);

	// 四个格子和箭头没有对应的资源控件，位置大小在这里一次性算好
	CalcSlotRects();


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDlgSmith::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}

void CDlgSmith::SetTextureStore(CItemTextureStore* pTextureStore)
{
	m_pTextureStore = pTextureStore;
}


BEGIN_MESSAGE_MAP(CDlgSmith, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


// CDlgSmith 消息处理程序

void CDlgSmith::CalcSlotRects()
{
	GetDlgItem(IDC_STATICSMITHGROUP)->ShowWindow(SW_HIDE);
	for (int slot = 0; slot < SLOT_OUTPUT; ++slot)
		m_rcSlots[slot] = m_ui.Rect(22 + slot * 100, 110, 70, 70);
	m_rcSlots[SLOT_OUTPUT] = m_ui.Rect(480, 110, 70, 70);
	m_rcArrow = m_ui.Rect(335, 110, 80, 70);
	for (int slot = 0; slot < SLOT_COUNT; ++slot)
	{
		m_ui.Move(IDC_STATICSMITHLABEL0 + slot, slot == SLOT_OUTPUT ? 480 : 22 + slot * 100, 184, 70, 27);
		InvalidateSlot(slot);
	}
	SetDlgItemText(IDC_STATICSMITHLABEL3, _T("物品"));
	SetDlgItemText(IDC_STATICSMITHHINT, _T("左键选择，右键清空；停留在格子上可查看 ID 和数量。"));
	m_ui.Move(IDC_STATICSMITHHINT, 22, 270, 656, 40);
}

CDlgSmith::SmithCell* CDlgSmith::GetSlot(int nSlot)
{
	// 复用常量版本，避免两处判断逻辑写得不一致
	return const_cast<SmithCell*>(static_cast<const CDlgSmith*>(this)->GetSlot(nSlot));
}

const CDlgSmith::SmithCell* CDlgSmith::GetSlot(int nSlot) const
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return nullptr;

	return &m_slots[nSlot];
}

BOOL CDlgSmith::HitTestSlot(CPoint point, int& nSlot) const
{
	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		if (m_rcSlots[i].PtInRect(point))
		{
			nSlot = i;
			return TRUE;
		}
	}

	return FALSE;
}

void CDlgSmith::InvalidateSlot(int nSlot)
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return;

	const auto* cell = GetSlot(nSlot);
	CString label;
	const LPCTSTR labels[] = { _T("模板"), _T("升级物品"), _T("原材料"), _T("产物") }; label = labels[nSlot];
	m_ui.Tip(nSlot + 1, m_rcSlots[nSlot], label, cell->strItem, cell->nCount);
	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgSmith::OnPaint()
{
	CPaintDC dc(this);
	DrawSlots(&dc);
}

void CDlgSmith::DrawSlots(CDC* pDC)
{
	m_ui.Text(pDC, _T("锻造台"), m_ui.Rect(22, 16, 656, 29));
	m_ui.Text(pDC, _T("原料（模板、升级物品与原材料）"), m_ui.Rect(22, 70, 420, 25));
	m_ui.Text(pDC, _T("产物"), m_ui.Rect(480, 70, 198, 25));
	m_ui.Arrow(pDC, m_rcArrow);
	for (int slot = 0; slot < SLOT_COUNT; ++slot)
		m_ui.Slot(pDC, m_rcSlots[slot], slot == m_nHoverSlot, m_slots[slot].strItem, m_slots[slot].nCount, m_pTextureStore);
}


void CDlgSmith::OnLButtonDown(UINT nFlags, CPoint point)
{
	int nSlot = -1;
	if (HitTestSlot(point, nSlot))
		SelectSlotItem(nSlot);

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgSmith::OnRButtonDown(UINT nFlags, CPoint point)
{
	int nSlot = -1;
	if (HitTestSlot(point, nSlot))
	{
		// 右键清空格子，只重画这一格
		SmithCell* pCell = GetSlot(nSlot);
		if (pCell != nullptr)
		{
			pCell->strItem.Empty();
			pCell->nCount = 0;
			InvalidateSlot(nSlot);
		}
	}

	CDialogEx::OnRButtonDown(nFlags, point);
}

void CDlgSmith::OnMouseMove(UINT nFlags, CPoint point)
{
	int nSlot = -1;
	if (!HitTestSlot(point, nSlot))
		nSlot = -1;

	if (nSlot != m_nHoverSlot)
	{
		// 高亮变化时只重画受影响的两个格子
		InvalidateSlot(m_nHoverSlot);
		m_nHoverSlot = nSlot;
		InvalidateSlot(m_nHoverSlot);
	}

	if (!m_bTrackingMouse)
	{
		// 请求鼠标移出通知，用来取消高亮
		TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = GetSafeHwnd();
		m_bTrackingMouse = _TrackMouseEvent(&tme);
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CDlgSmith::OnMouseLeave()
{
	m_bTrackingMouse = FALSE;

	if (m_nHoverSlot >= 0)
	{
		const int nSlot = m_nHoverSlot;
		m_nHoverSlot = -1;
		InvalidateSlot(nSlot);
	}

	CDialogEx::OnMouseLeave();
}

BOOL CDlgSmith::SelectSlotItem(int nSlot)
{
	// 还没有导入任何物品时先提示，免得弹出一个空列表
	if (m_pItemSource == nullptr || m_pItemSource->GetSize() == 0)
	{
		AfxMessageBox(_T("还没有可选的物品，请先在主界面点击“导入 jar 文件”。"), MB_ICONINFORMATION);
		return FALSE;
	}

	CDlgSelect dlgSelect;
	dlgSelect.SetItemList(m_pItemSource);

	// 格子里已经有物品时，带着原来的物品和数量打开，方便修改
	const SmithCell* pCell = GetSlot(nSlot);
	if (pCell != nullptr && !pCell->IsEmpty())
		dlgSelect.SetInitialSelection(pCell->strItem, pCell->nCount);

	if (dlgSelect.DoModal() != IDOK)
		return FALSE;

	SmithCell* pTarget = GetSlot(nSlot);
	if (pTarget != nullptr)
	{
		pTarget->strItem = dlgSelect.GetSelectedItem();
		pTarget->nCount = dlgSelect.GetSelectedCount();
		InvalidateSlot(nSlot);
	}

	return TRUE;
}

void CDlgSmith::OnOK()
{
	// 四个格子都选好才能生成脚本
	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		if (m_slots[i].strItem.IsEmpty())
		{
			AfxMessageBox(_T("请先点击四个格子，把模板、升级物品、材料和产物都选好。"), MB_ICONINFORMATION);
			return;
		}
	}

	CDialogEx::OnOK();
}

CString CDlgSmith::BuildRecipeScript() const
{
	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		if (m_slots[i].strItem.IsEmpty())
			return CString();
	}

	// 产物：数量大于 1 时写成 Item.of(...)
	const SmithCell& out = m_slots[SLOT_OUTPUT];
	CString strResult;
	if (out.nCount > 1)
		strResult.Format(_T("Item.of('%s', %d)"), out.strItem.GetString(), out.nCount);
	else
		strResult.Format(_T("'%s'"), out.strItem.GetString());

	// event.smithing(产物, 模板, 被升级物品, 材料)
	CString strScript;
	strScript.Format(
		_T("ServerEvents.recipes(event => {\r\n")
		_T("  // 锻造台 %s\r\n")
		_T("  event.smithing(\r\n")
		_T("    %s,\r\n")
		_T("    '%s',\r\n")
		_T("    '%s',\r\n")
		_T("    '%s'\r\n")
		_T("  )\r\n")
		_T("})\r\n"),
		out.strItem.GetString(), strResult.GetString(),
		m_slots[SLOT_TEMPLATE].strItem.GetString(),
		m_slots[SLOT_UPGRADE].strItem.GetString(),
		m_slots[SLOT_INGREDIENT].strItem.GetString());

	return strScript;
}

BOOL CDlgSmith::PreTranslateMessage(MSG* message)
{
	m_ui.Relay(message);
	return CDialogEx::PreTranslateMessage(message);
}
