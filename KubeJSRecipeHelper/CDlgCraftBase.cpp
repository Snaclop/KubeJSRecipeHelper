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

namespace
{
	// 格子的摆放尺寸（像素）
	const int SLOT_MARGIN   = 24;	// 格子区域到对话框左边、上边的距离
	const int HINT_GAP      = 85;	// 格子区域与提示文字之间的间距
	const int HINT_HEIGHT   = 18;	// 提示文字占用的高度
	const int GRID_MAX_CELL = 100;	// 单个格子的边长上限
	const int GRID_MIN_CELL = 8;	// 单个格子的边长下限

	// 仿照 Minecraft 物品栏格子的配色
	const COLORREF CR_SLOT       = RGB(0x8B, 0x8B, 0x8B);	// 格子底色
	const COLORREF CR_SLOT_HOVER = RGB(0xB0, 0xB0, 0xB0);	// 鼠标悬停时的格子底色
	const COLORREF CR_DARK       = RGB(0x37, 0x37, 0x37);	// 左上暗边
	const COLORREF CR_LIGHT      = RGB(0xFF, 0xFF, 0xFF);	// 右下亮边
	const COLORREF CR_ITEM       = RGB(0x20, 0x20, 0x20);	// 物品名文字
	const COLORREF CR_NAMESPACE  = RGB(0x3F, 0x3F, 0x3F);	// 命名空间文字
	const COLORREF CR_COUNT      = RGB(0xFF, 0xFF, 0xFF);	// 数量文字
}


// CDlgCraftBase 对话框

IMPLEMENT_DYNAMIC(CDlgCraftBase, CDialogEx)

CDlgCraftBase::CDlgCraftBase(UINT nIDTemplate, CWnd* pParent /*=nullptr*/)
	: CDialogEx(nIDTemplate, pParent)
	, m_rcSlotArea(0, 0, 0, 0)
	, m_rcHint(0, 0, 0, 0)
	, m_nCellSize(0)
	, m_nHoverSlot(-1)
	, m_bTrackingMouse(FALSE)
	, m_pItemSource(nullptr)
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

	// 九宫格和输出格没有对应的资源控件，位置大小在这里一次性算好
	CalcSlotRects();

	// 物品名一般比较长，单独准备一个小号字体画命名空间和数量
	CFont* pFont = GetFont();
	LOGFONT lf;
	if (pFont != nullptr && pFont->GetLogFont(&lf))
	{
		lf.lfHeight = lf.lfHeight * 3 / 4;
		lf.lfWidth = lf.lfWidth * 3 / 4;
		m_fontSmall.CreateFontIndirect(&lf);
	}

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
	CRect rcClient;
	GetClientRect(&rcClient);

	// 对话框下方是“确定/取消”按钮，用按钮的上边缘作为可用区域的下界
	int nBottom = rcClient.bottom;
	CWnd* pButton = GetDlgItem(IDOK);
	if (pButton != nullptr)
	{
		CRect rcButton;
		pButton->GetWindowRect(&rcButton);
		ScreenToClient(&rcButton);
		nBottom = rcButton.top;
	}

	const int nAvailH = nBottom - SLOT_MARGIN * 2 - HINT_GAP - HINT_HEIGHT;
	const int nAvailW = rcClient.Width() - SLOT_MARGIN * 2;

	// 横向要放下九宫格（3 格）+ 箭头（按 2/3 格算）+ 输出格（1 格），
	// 也就是 14/3 个格子宽
	int nCell = nAvailH / GRID_ROWS;
	const int nCellByWidth = nAvailW * 3 / 14;
	if (nCellByWidth < nCell)
		nCell = nCellByWidth;
	if (nCell > GRID_MAX_CELL)
		nCell = GRID_MAX_CELL;
	if (nCell < GRID_MIN_CELL)
		nCell = GRID_MIN_CELL;

	m_nCellSize = nCell;

	// 九宫格
	for (int nRow = 0; nRow < GRID_ROWS; ++nRow)
	{
		for (int nCol = 0; nCol < GRID_COLS; ++nCol)
		{
			CRect& rect = m_rcSlots[nRow * GRID_COLS + nCol];
			rect.SetRect(SLOT_MARGIN + nCol * nCell, SLOT_MARGIN + nRow * nCell,
				SLOT_MARGIN + (nCol + 1) * nCell, SLOT_MARGIN + (nRow + 1) * nCell);
		}
	}

	// 输出格：放在九宫格右边，竖直方向跟九宫格中间那行对齐（跟 MC 工作台一样）
	const int nArrowWidth = nCell * 2 / 3;
	const int nOutputLeft = m_rcSlots[GRID_COLS - 1].right + nArrowWidth;
	const int nOutputTop = SLOT_MARGIN + nCell;
	m_rcSlots[SLOT_OUTPUT].SetRect(nOutputLeft, nOutputTop,
		nOutputLeft + nCell, nOutputTop + nCell);

	// 整块区域（含中间的箭头），重画时先把它铺满
	m_rcSlotArea.SetRect(m_rcSlots[0].left, m_rcSlots[0].top,
		m_rcSlots[SLOT_OUTPUT].right, m_rcSlots[SLOT_OUTPUT].bottom);

	m_rcHint.SetRect(m_rcSlotArea.left, m_rcSlotArea.bottom + HINT_GAP,
		m_rcSlotArea.right, m_rcSlotArea.bottom + HINT_GAP + HINT_HEIGHT);
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

	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgCraftBase::OnPaint()
{
	CDialogEx::OnPaint();	// 先让对话框把背景擦好

	CClientDC dc(this);
	DrawSlots(&dc);
}

void CDlgCraftBase::DrawSlots(CDC* pDC)
{
	CFont* pFont = GetFont();
	CFont* pOldFont = (pFont != nullptr) ? pDC->SelectObject(pFont) : nullptr;

	// 自己把整块区域铺满，这样重画格子时不用依赖对话框的擦除
	pDC->FillSolidRect(m_rcSlotArea, GetSysColor(COLOR_3DFACE));

	// 九宫格和输出格之间的箭头
	DrawArrow(pDC);

	for (int nSlot = 0; nSlot < SLOT_COUNT; ++nSlot)
	{
		DrawSlotFrame(pDC, m_rcSlots[nSlot], nSlot == m_nHoverSlot);

		const CraftCell* pCell = GetSlot(nSlot);
		if (pCell != nullptr && !pCell->IsEmpty())
			DrawSlotItem(pDC, m_rcSlots[nSlot], pCell->strItem, pCell->nCount);
	}

	// 操作提示
	pDC->FillSolidRect(m_rcHint, GetSysColor(COLOR_3DFACE));
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
	pDC->DrawText(GetHintText(), m_rcHint, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	if (pOldFont != nullptr)
		pDC->SelectObject(pOldFont);
}

void CDlgCraftBase::DrawSlotFrame(CDC* pDC, const CRect& rect, BOOL bHover)
{
	// 方格：底色 + 左上暗边、右下亮边，做出 MC 物品栏格子的凹陷效果
	// 相邻方格紧挨着画，对方的亮边/暗边正好拼成分隔线
	pDC->FillSolidRect(rect, bHover ? CR_SLOT_HOVER : CR_SLOT);
	pDC->FillSolidRect(rect.left, rect.top, rect.Width() - 1, 1, CR_DARK);
	pDC->FillSolidRect(rect.left, rect.top, 1, rect.Height() - 1, CR_DARK);
	pDC->FillSolidRect(rect.left + 1, rect.bottom - 1, rect.Width() - 1, 1, CR_LIGHT);
	pDC->FillSolidRect(rect.right - 1, rect.top + 1, 1, rect.Height() - 1, CR_LIGHT);
}

void CDlgCraftBase::DrawArrow(CDC* pDC)
{
	// 九宫格和输出格之间画一个白色箭头，跟 MC 工作台一样指示“合成”
	const CRect& rcOutput = m_rcSlots[SLOT_OUTPUT];
	const CRect rcArrow(m_rcSlots[GRID_COLS - 1].right, rcOutput.top, rcOutput.left, rcOutput.bottom);
	if (rcArrow.Width() < 8 || rcArrow.Height() < 8)
		return;

	const int nCenterY = rcArrow.CenterPoint().y;
	const int nShaftHalf = (m_nCellSize / 16 > 1) ? m_nCellSize / 16 : 1;	// 箭杆的半高
	const int nHeadHalf = (m_nCellSize / 6 > 2) ? m_nCellSize / 6 : 2;		// 箭头三角的半高
	const int nShaftEnd = rcArrow.left + rcArrow.Width() * 2 / 3;			// 箭杆画到这里

	POINT ptsArrow[7] =
	{
		{ rcArrow.left,   nCenterY - nShaftHalf },
		{ nShaftEnd,      nCenterY - nShaftHalf },
		{ nShaftEnd,      nCenterY - nHeadHalf },
		{ rcArrow.right,  nCenterY },
		{ nShaftEnd,      nCenterY + nHeadHalf },
		{ nShaftEnd,      nCenterY + nShaftHalf },
		{ rcArrow.left,   nCenterY + nShaftHalf }
	};

	CPen pen(PS_SOLID, 1, CR_DARK);
	CBrush brush(CR_LIGHT);
	CPen* pOldPen = pDC->SelectObject(&pen);
	CBrush* pOldBrush = pDC->SelectObject(&brush);

	pDC->Polygon(ptsArrow, _countof(ptsArrow));

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

void CDlgCraftBase::DrawSlotItem(CDC* pDC, const CRect& rect, const CString& strItem, int nCount)
{
	// 物品 id 形如“命名空间:名称”，拆开显示：上面是命名空间，中间是名称
	CString strNamespace;
	CString strName;
	const int nColon = strItem.Find(_T(':'));
	if (nColon > 0)
	{
		strNamespace = strItem.Left(nColon);
		strName = strItem.Mid(nColon + 1);
	}
	else
	{
		strName = strItem;
	}

	CFont* pFont = GetFont();
	pDC->SetBkMode(TRANSPARENT);

	CRect rcText(rect);
	rcText.DeflateRect(4, 3);

	// 顶部一行小字：命名空间（也就是模组名）
	if (!strNamespace.IsEmpty() && m_fontSmall.GetSafeHandle() != nullptr)
	{
		CFont* pOldFont = pDC->SelectObject(&m_fontSmall);
		const CSize sizeText = pDC->GetTextExtent(strNamespace);
		CRect rcNamespace(rcText.left, rcText.top, rcText.right, rcText.top + sizeText.cy);

		pDC->SetTextColor(CR_NAMESPACE);
		pDC->DrawText(strNamespace, rcNamespace, DT_SINGLELINE | DT_CENTER | DT_TOP | DT_END_ELLIPSIS);
		pDC->SelectObject(pOldFont);

		rcText.top = rcNamespace.bottom;
	}

	// 中间：物品名称，太长时自动折行并做垂直居中
	CRect rcName(rcText);
	if (nCount > 1)
		rcName.bottom -= 10;	// 给右下角的数量让位

	if (rcName.bottom > rcName.top)
	{
		CFont* pOldFont = (pFont != nullptr) ? pDC->SelectObject(pFont) : nullptr;

		// 先算出折行后需要的高度，再把它挪到垂直居中的位置
		CRect rcCalc(0, 0, rcName.Width(), 0);
		pDC->DrawText(strName, rcCalc, DT_CALCRECT | DT_WORDBREAK | DT_CENTER);
		if (rcCalc.Height() < rcName.Height())
			rcName.top += (rcName.Height() - rcCalc.Height()) / 2;

		pDC->SetTextColor(CR_ITEM);
		pDC->DrawText(strName, rcName, DT_WORDBREAK | DT_CENTER);

		if (pOldFont != nullptr)
			pDC->SelectObject(pOldFont);
	}

	// 右下角：数量（MC 里数量是带阴影的白色数字）
	if (nCount > 1 && m_fontSmall.GetSafeHandle() != nullptr)
	{
		CString strCount;
		strCount.Format(_T("%d"), nCount);

		CFont* pOldFont = pDC->SelectObject(&m_fontSmall);
		const CSize sizeText = pDC->GetTextExtent(strCount);
		CRect rcCount(rect.right - sizeText.cx - 4, rect.bottom - sizeText.cy - 3,
			rect.right - 2, rect.bottom - 1);

		CRect rcShadow(rcCount);
		rcShadow.OffsetRect(1, 1);

		pDC->SetTextColor(CR_DARK);
		pDC->DrawText(strCount, rcShadow, DT_SINGLELINE | DT_LEFT | DT_TOP);

		pDC->SetTextColor(CR_COUNT);
		pDC->DrawText(strCount, rcCount, DT_SINGLELINE | DT_LEFT | DT_TOP);
		pDC->SelectObject(pOldFont);
	}
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
