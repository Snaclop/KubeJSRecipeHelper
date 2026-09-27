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

namespace
{
	// 格子与“输入”分组框、说明文字之间的间距（像素）
	const int GROUP_TITLE_H = 16;	// 分组框标题占用的高度
	const int SLOT_GAP      = 6;	// 分组框内边距、说明文字上方的留白
	const int SLOT_GAP_H    = 10;	// 格子之间的水平间距
	const int LABEL_GAP     = 4;	// 格子与下面一行说明文字的间距
	const int SLOT_MAX      = 100;	// 单个格子的边长上限
	const int SLOT_MIN      = 8;	// 单个格子的边长下限

	// 仿照 Minecraft 物品栏格子的配色（与有序、无序合成保持一致）
	const COLORREF CR_SLOT       = RGB(0x8B, 0x8B, 0x8B);	// 格子底色
	const COLORREF CR_SLOT_HOVER = RGB(0xB0, 0xB0, 0xB0);	// 鼠标悬停时的格子底色
	const COLORREF CR_DARK       = RGB(0x37, 0x37, 0x37);	// 左上暗边
	const COLORREF CR_LIGHT      = RGB(0xFF, 0xFF, 0xFF);	// 右下亮边
	const COLORREF CR_ITEM       = RGB(0x20, 0x20, 0x20);	// 物品名文字
	const COLORREF CR_NAMESPACE  = RGB(0x3F, 0x3F, 0x3F);	// 命名空间文字
	const COLORREF CR_COUNT      = RGB(0xFF, 0xFF, 0xFF);	// 数量文字
}


// CDlgSmith 对话框

IMPLEMENT_DYNAMIC(CDlgSmith, CDialogEx)

CDlgSmith::CDlgSmith(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGSMITH, pParent)
	, m_nCellSize(0)
	, m_nHoverSlot(-1)
	, m_bTrackingMouse(FALSE)
	, m_pItemSource(nullptr)
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

	// 四个格子和箭头没有对应的资源控件，位置大小在这里一次性算好
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

void CDlgSmith::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
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
	// “输入”分组框和它下面那行说明文字在资源里已经摆好，运行期不会再动，
	// 这里用它们的位置来算格子摆哪：三个输入格摆在框里，输出格放在框右边
	CRect rcGroup(0, 0, 0, 0);
	if (CWnd* pGroup = GetDlgItem(IDC_STATICSMITHGROUP))
	{
		pGroup->GetWindowRect(&rcGroup);
		ScreenToClient(&rcGroup);
	}

	CRect rcLabel(0, 0, 0, 0);
	if (CWnd* pLabel = GetDlgItem(IDC_STATICSMITHLABEL0))
	{
		pLabel->GetWindowRect(&rcLabel);
		ScreenToClient(&rcLabel);
	}

	// 竖直方向：上面让出分组框标题的高度，下面给说明文字那一行留位置
	const int nTop = rcGroup.top + GROUP_TITLE_H + SLOT_GAP;
	const int nBottom = rcLabel.top - SLOT_GAP;

	// 横向：三个输入格摆一排，格子之间留缝
	const int nInputCount = SLOT_OUTPUT - SLOT_TEMPLATE;
	int nCell = (rcGroup.Width() - SLOT_GAP * 2 - SLOT_GAP_H * (nInputCount - 1)) / nInputCount;

	const int nCellAreaH = nBottom - nTop;
	if (nCell > nCellAreaH)
		nCell = nCellAreaH;
	if (nCell > SLOT_MAX)
		nCell = SLOT_MAX;
	if (nCell < SLOT_MIN)
		nCell = SLOT_MIN;

	m_nCellSize = nCell;

	// 三个输入格在分组框里水平居中
	const int nRowW = nCell * nInputCount + SLOT_GAP_H * (nInputCount - 1);
	const int nRowTop = nTop + (nCellAreaH - nCell) / 2;
	int nLeft = rcGroup.left + (rcGroup.Width() - nRowW) / 2;

	for (int nSlot = SLOT_TEMPLATE; nSlot <= SLOT_INGREDIENT; ++nSlot)
	{
		m_rcSlots[nSlot].SetRect(nLeft, nRowTop, nLeft + nCell, nRowTop + nCell);
		nLeft += nCell + SLOT_GAP_H;
	}

	// 输出格：跟三个输入格同一行，中间留出画箭头的位置
	const int nArrowWidth = nCell * 2 / 3;
	const int nOutputLeft = rcGroup.right + nArrowWidth;
	m_rcSlots[SLOT_OUTPUT].SetRect(nOutputLeft, nRowTop, nOutputLeft + nCell, nRowTop + nCell);

	// 箭头：夹在分组框和输出格之间，左边避开分组框的边框
	m_rcArrow.SetRect(rcGroup.right + 1, nRowTop, nOutputLeft, nRowTop + nCell);

	// 四个说明文字是对着格子的一行，跟着格子的位置走，格子大小变了也能对得齐
	for (int nSlot = 0; nSlot < SLOT_COUNT; ++nSlot)
	{
		CWnd* pLabel = GetDlgItem(IDC_STATICSMITHLABEL0 + nSlot);
		if (pLabel != nullptr)
		{
			pLabel->SetWindowPos(nullptr,
				m_rcSlots[nSlot].left, m_rcSlots[nSlot].bottom + LABEL_GAP,
				nCell, rcLabel.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
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

	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgSmith::OnPaint()
{
	CDialogEx::OnPaint();	// 先让对话框把背景擦好

	CClientDC dc(this);
	DrawSlots(&dc);
}

void CDlgSmith::DrawSlots(CDC* pDC)
{
	CFont* pFont = GetFont();
	CFont* pOldFont = (pFont != nullptr) ? pDC->SelectObject(pFont) : nullptr;

	// 格子底下是对话框自己的底色，这里只把箭头那一块铺掉，
	// 免得重画箭头时留下上一帧的痕迹
	pDC->FillSolidRect(m_rcArrow, GetSysColor(COLOR_3DFACE));
	DrawArrow(pDC);

	for (int nSlot = 0; nSlot < SLOT_COUNT; ++nSlot)
	{
		DrawSlotFrame(pDC, m_rcSlots[nSlot], nSlot == m_nHoverSlot);

		const SmithCell* pCell = GetSlot(nSlot);
		if (pCell != nullptr && !pCell->IsEmpty())
			DrawSlotItem(pDC, m_rcSlots[nSlot], pCell->strItem, pCell->nCount);
	}

	if (pOldFont != nullptr)
		pDC->SelectObject(pOldFont);
}

void CDlgSmith::DrawSlotFrame(CDC* pDC, const CRect& rect, BOOL bHover)
{
	// 方格：底色 + 左上暗边、右下亮边，做出 MC 物品栏格子的凹陷效果
	pDC->FillSolidRect(rect, bHover ? CR_SLOT_HOVER : CR_SLOT);
	pDC->FillSolidRect(rect.left, rect.top, rect.Width() - 1, 1, CR_DARK);
	pDC->FillSolidRect(rect.left, rect.top, 1, rect.Height() - 1, CR_DARK);
	pDC->FillSolidRect(rect.left + 1, rect.bottom - 1, rect.Width() - 1, 1, CR_LIGHT);
	pDC->FillSolidRect(rect.right - 1, rect.top + 1, 1, rect.Height() - 1, CR_LIGHT);
}

void CDlgSmith::DrawArrow(CDC* pDC)
{
	// 输入格和输出格之间画一个白色箭头，跟 MC 锻造台一样指示“升级”
	if (m_rcArrow.Width() < 8 || m_rcArrow.Height() < 8)
		return;

	const int nCenterY = m_rcArrow.CenterPoint().y;
	const int nShaftHalf = (m_nCellSize / 16 > 1) ? m_nCellSize / 16 : 1;	// 箭杆的半高
	const int nHeadHalf = (m_nCellSize / 6 > 2) ? m_nCellSize / 6 : 2;		// 箭头三角的半高
	const int nShaftEnd = m_rcArrow.left + m_rcArrow.Width() * 2 / 3;		// 箭杆画到这里

	POINT ptsArrow[7] =
	{
		{ m_rcArrow.left,   nCenterY - nShaftHalf },
		{ nShaftEnd,        nCenterY - nShaftHalf },
		{ nShaftEnd,        nCenterY - nHeadHalf },
		{ m_rcArrow.right,  nCenterY },
		{ nShaftEnd,        nCenterY + nHeadHalf },
		{ nShaftEnd,        nCenterY + nShaftHalf },
		{ m_rcArrow.left,   nCenterY + nShaftHalf }
	};

	CPen pen(PS_SOLID, 1, CR_DARK);
	CBrush brush(CR_LIGHT);
	CPen* pOldPen = pDC->SelectObject(&pen);
	CBrush* pOldBrush = pDC->SelectObject(&brush);

	pDC->Polygon(ptsArrow, _countof(ptsArrow));

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

void CDlgSmith::DrawSlotItem(CDC* pDC, const CRect& rect, const CString& strItem, int nCount)
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