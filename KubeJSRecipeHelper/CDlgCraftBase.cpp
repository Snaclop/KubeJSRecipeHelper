// CDlgCraftBase.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgCraftBase.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDlgCraftBase 对话框

IMPLEMENT_DYNAMIC(CDlgCraftBase, CDialogEx)

CDlgCraftBase::CDlgCraftBase(UINT nIDTemplate, CWnd* pParent /*=nullptr*/)
	: CDialogEx(nIDTemplate, pParent)
	, m_rcHint(0, 0, 0, 0)
	, m_nHoverSlot(-1)
	, m_bTrackingMouse(FALSE)
	, m_pItemSource(nullptr)
	, m_pTextureStore(nullptr)
{

}

CDlgCraftBase::~CDlgCraftBase()
{
}

void CDlgCraftBase::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CDlgCraftBase::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}

void CDlgCraftBase::SetTextureStore(CItemTextureStore* pTextureStore)
{
	m_pTextureStore = pTextureStore;
}

BEGIN_MESSAGE_MAP(CDlgCraftBase, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


// CDlgCraftBase 消息处理程序

BOOL CDlgCraftBase::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_ui.Initialize(this, 700, 440);

	// 九宫格和输出格没有对应的资源控件，位置大小在这里一次性算好
	CalcSlotRects();


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDlgCraftBase::OnOK()
{
	CString strError;
	if (!ValidateSlots(strError))
	{
		AfxMessageBox(strError, MB_ICONINFORMATION);
		return;
	}

	CDialogEx::OnOK();
}

LPCTSTR CDlgCraftBase::GetHintText() const
{
	return _T("左键点击格子选择物品，右键清除；点“确定”生成 KubeJS 脚本");
}

BOOL CDlgCraftBase::ValidateSlots(CString& strError) const
{
	if (m_outputCell.IsEmpty())
	{
		strError = _T("请先点击最右边的输出格，选择合成产物。");
		return FALSE;
	}

	BOOL bHasMaterial = FALSE;
	for (int nRow = 0; nRow < GRID_ROWS && !bHasMaterial; ++nRow)
	{
		for (int nCol = 0; nCol < GRID_COLS && !bHasMaterial; ++nCol)
			bHasMaterial = !m_cells[nRow][nCol].IsEmpty();
	}

	if (!bHasMaterial)
	{
		strError = _T("请先在九宫格里放上材料。");
		return FALSE;
	}

	return TRUE;
}

void CDlgCraftBase::CalcSlotRects()
{
	// 原料区与产物区的中心分别为 185、515，关于窗口中心 350 对称。
	for (int row = 0; row < GRID_ROWS; ++row)
		for (int col = 0; col < GRID_COLS; ++col)
			m_rcSlots[row * GRID_COLS + col] = m_ui.Rect(89 + col * 67, 110 + row * 67, 58, 58);
	m_rcSlots[SLOT_OUTPUT] = m_ui.Rect(480, 171, 70, 70);
	m_rcHint = m_ui.Rect(22, 350, 656, 42);
	for (int slot = 0; slot < SLOT_COUNT; ++slot) InvalidateSlot(slot);
}

CDlgCraftBase::CraftCell* CDlgCraftBase::GetSlot(int nSlot)
{
	// 复用常量版本，避免两处判断逻辑写得不一致
	return const_cast<CraftCell*>(static_cast<const CDlgCraftBase*>(this)->GetSlot(nSlot));
}

const CDlgCraftBase::CraftCell* CDlgCraftBase::GetSlot(int nSlot) const
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return nullptr;
	if (nSlot == SLOT_OUTPUT)
		return &m_outputCell;

	return &m_cells[nSlot / GRID_COLS][nSlot % GRID_COLS];
}

BOOL CDlgCraftBase::HitTestSlot(CPoint point, int& nSlot) const
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

void CDlgCraftBase::InvalidateSlot(int nSlot)
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return;

	const auto* cell = GetSlot(nSlot);
	CString label;
	label.Format(nSlot == SLOT_OUTPUT ? _T("产物") : _T("原料 %d"), nSlot + 1);
	m_ui.Tip(nSlot + 1, m_rcSlots[nSlot], label, cell->strItem, cell->nCount);
	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgCraftBase::OnPaint()
{
	CPaintDC dc(this);
	DrawSlots(&dc);
}

void CDlgCraftBase::DrawSlots(CDC* pDC)
{
	CString title; GetWindowText(title);
	m_ui.Text(pDC, title, m_ui.Rect(22, 16, 656, 29));
	m_ui.Text(pDC, _T("合成网格（3 × 3）"), m_ui.Rect(89, 70, 192, 25), false, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	m_ui.Text(pDC, _T("产物"), m_ui.Rect(480, 70, 70, 25), false, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	m_ui.Arrow(pDC, m_ui.Rect(320, 171, 120, 70));
	for (int slot = 0; slot < SLOT_COUNT; ++slot)
	{
		const CraftCell* cell = GetSlot(slot);
		m_ui.Slot(pDC, m_rcSlots[slot], slot == m_nHoverSlot, cell->strItem, cell->nCount, m_pTextureStore);
	}
	m_ui.Text(pDC, _T("左键选择，右键清空；停留在格子上可查看 ID 和数量。"), m_ui.Rect(22, 316, 656, 27), true);
	m_ui.Text(pDC, GetHintText(), m_rcHint, true, DT_LEFT | DT_WORDBREAK);
}


void CDlgCraftBase::OnLButtonDown(UINT nFlags, CPoint point)
{
	int nSlot = -1;
	if (HitTestSlot(point, nSlot))
		SelectSlotItem(nSlot);

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgCraftBase::OnRButtonDown(UINT nFlags, CPoint point)
{
	int nSlot = -1;
	if (HitTestSlot(point, nSlot))
	{
		// 右键清空格子，只重画这一格
		CraftCell* pCell = GetSlot(nSlot);
		if (pCell != nullptr)
		{
			pCell->strItem.Empty();
			pCell->nCount = 0;
			InvalidateSlot(nSlot);
		}
	}

	CDialogEx::OnRButtonDown(nFlags, point);
}

void CDlgCraftBase::OnMouseMove(UINT nFlags, CPoint point)
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

void CDlgCraftBase::OnMouseLeave()
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

BOOL CDlgCraftBase::SelectSlotItem(int nSlot)
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
	const CraftCell* pCell = GetSlot(nSlot);
	if (pCell != nullptr && !pCell->IsEmpty())
		dlgSelect.SetInitialSelection(pCell->strItem, pCell->nCount);

	if (dlgSelect.DoModal() != IDOK)
		return FALSE;

	CraftCell* pTarget = GetSlot(nSlot);
	if (pTarget != nullptr)
	{
		pTarget->strItem = dlgSelect.GetSelectedItem();
		pTarget->nCount = dlgSelect.GetSelectedCount();
		InvalidateSlot(nSlot);
	}

	return TRUE;
}

BOOL CDlgCraftBase::PreTranslateMessage(MSG* message)
{
	m_ui.Relay(message);
	return CDialogEx::PreTranslateMessage(message);
}
