// CDlgShaped.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgShaped.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	// 九宫格的摆放尺寸（像素）
	const int GRID_MARGIN   = 24;	// 九宫格到对话框左边、上边的距离
	const int HINT_GAP      = 10;	// 九宫格与提示文字之间的间距
	const int HINT_HEIGHT   = 18;	// 提示文字占用的高度
	const int GRID_MAX_CELL = 100;	// 单个方格的边长上限
	const int GRID_MIN_CELL = 8;	// 单个方格的边长下限

	// 仿照 Minecraft 物品栏格子的配色
	const COLORREF CR_SLOT       = RGB(0x8B, 0x8B, 0x8B);	// 方格底色
	const COLORREF CR_SLOT_HOVER = RGB(0xB0, 0xB0, 0xB0);	// 鼠标悬停时的方格底色
	const COLORREF CR_DARK       = RGB(0x37, 0x37, 0x37);	// 左上暗边
	const COLORREF CR_LIGHT      = RGB(0xFF, 0xFF, 0xFF);	// 右下亮边
	const COLORREF CR_ITEM       = RGB(0x20, 0x20, 0x20);	// 物品名文字
	const COLORREF CR_NAMESPACE  = RGB(0x3F, 0x3F, 0x3F);	// 命名空间文字
	const COLORREF CR_COUNT      = RGB(0xFF, 0xFF, 0xFF);	// 数量文字
}


// CDlgShaped 对话框

IMPLEMENT_DYNAMIC(CDlgShaped, CDialogEx)

CDlgShaped::CDlgShaped(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGSHAPED, pParent)
	, m_rcGrid(0, 0, 0, 0)
	, m_rcHint(0, 0, 0, 0)
	, m_nCellSize(0)
	, m_nHoverRow(-1)
	, m_nHoverCol(-1)
	, m_bTrackingMouse(FALSE)
	, m_pItemSource(nullptr)
{

}

CDlgShaped::~CDlgShaped()
{
}

void CDlgShaped::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CDlgShaped::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}

BEGIN_MESSAGE_MAP(CDlgShaped, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


// CDlgShaped 消息处理程序

BOOL CDlgShaped::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 九宫格没有对应的资源控件，位置大小在这里一次性算好
	CalcGridRect();

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

void CDlgShaped::CalcGridRect()
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

	const int nAvailH = nBottom - GRID_MARGIN * 2 - HINT_GAP - HINT_HEIGHT;
	const int nAvailW = rcClient.Width() - GRID_MARGIN * 2;

	// 先按可用高度取边长，再受宽度和上下限约束，保证九宫格始终是正方形
	int nCell = nAvailH / GRID_ROWS;
	if (nAvailW / GRID_COLS < nCell)
		nCell = nAvailW / GRID_COLS;
	if (nCell > GRID_MAX_CELL)
		nCell = GRID_MAX_CELL;
	if (nCell < GRID_MIN_CELL)
		nCell = GRID_MIN_CELL;

	m_nCellSize = nCell;
	m_rcGrid.SetRect(GRID_MARGIN, GRID_MARGIN,
		GRID_MARGIN + nCell * GRID_COLS, GRID_MARGIN + nCell * GRID_ROWS);
	m_rcHint.SetRect(m_rcGrid.left, m_rcGrid.bottom + HINT_GAP,
		m_rcGrid.right, m_rcGrid.bottom + HINT_GAP + HINT_HEIGHT);
}

void CDlgShaped::GetCellRect(int nRow, int nCol, CRect& rect) const
{
	const int nLeft = m_rcGrid.left + nCol * m_nCellSize;
	const int nTop = m_rcGrid.top + nRow * m_nCellSize;
	rect.SetRect(nLeft, nTop, nLeft + m_nCellSize, nTop + m_nCellSize);
}

BOOL CDlgShaped::HitTestCell(CPoint point, int& nRow, int& nCol) const
{
	if (m_nCellSize <= 0 || !m_rcGrid.PtInRect(point))
		return FALSE;

	nCol = (point.x - m_rcGrid.left) / m_nCellSize;
	nRow = (point.y - m_rcGrid.top) / m_nCellSize;

	return nRow >= 0 && nRow < GRID_ROWS && nCol >= 0 && nCol < GRID_COLS;
}

void CDlgShaped::InvalidateCell(int nRow, int nCol)
{
	if (m_nCellSize <= 0 || nRow < 0 || nRow >= GRID_ROWS || nCol < 0 || nCol >= GRID_COLS)
		return;

	CRect rect;
	GetCellRect(nRow, nCol, rect);
	InvalidateRect(&rect, FALSE);
}

void CDlgShaped::OnPaint()
{
	CDialogEx::OnPaint();	// 先让对话框把背景擦好

	CClientDC dc(this);
	DrawGrid(&dc);
}

void CDlgShaped::DrawGrid(CDC* pDC)
{
	CFont* pFont = GetFont();
	CFont* pOldFont = (pFont != nullptr) ? pDC->SelectObject(pFont) : nullptr;

	// 自己把九宫格占用的区域铺满，这样重画格子时不用依赖对话框的擦除
	pDC->FillSolidRect(m_rcGrid, GetSysColor(COLOR_3DFACE));

	for (int nRow = 0; nRow < GRID_ROWS; ++nRow)
	{
		for (int nCol = 0; nCol < GRID_COLS; ++nCol)
		{
			CRect rect;
			GetCellRect(nRow, nCol, rect);
			DrawCell(pDC, nRow, nCol, rect, nRow == m_nHoverRow && nCol == m_nHoverCol);
		}
	}

	// 操作提示
	pDC->FillSolidRect(m_rcHint, GetSysColor(COLOR_3DFACE));
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
	pDC->DrawText(_T("左键点击方格选择物品，右键点击方格清除"), m_rcHint,
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	if (pOldFont != nullptr)
		pDC->SelectObject(pOldFont);
}

void CDlgShaped::DrawCell(CDC* pDC, int nRow, int nCol, const CRect& rect, BOOL bHover)
{
	// 方格：底色 + 左上暗边、右下亮边，做出 MC 物品栏格子的凹陷效果
	// 相邻方格紧挨着画，对方的亮边/暗边正好拼成分隔线
	pDC->FillSolidRect(rect, bHover ? CR_SLOT_HOVER : CR_SLOT);
	pDC->FillSolidRect(rect.left, rect.top, rect.Width() - 1, 1, CR_DARK);
	pDC->FillSolidRect(rect.left, rect.top, 1, rect.Height() - 1, CR_DARK);
	pDC->FillSolidRect(rect.left + 1, rect.bottom - 1, rect.Width() - 1, 1, CR_LIGHT);
	pDC->FillSolidRect(rect.right - 1, rect.top + 1, 1, rect.Height() - 1, CR_LIGHT);

	const CraftCell& cell = m_cells[nRow][nCol];
	if (!cell.IsEmpty())
		DrawCellItem(pDC, rect, cell.strItem, cell.nCount);
}

void CDlgShaped::DrawCellItem(CDC* pDC, const CRect& rect, const CString& strItem, int nCount)
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

void CDlgShaped::OnLButtonDown(UINT nFlags, CPoint point)
{
	int nRow = -1;
	int nCol = -1;
	if (HitTestCell(point, nRow, nCol))
		SelectCellItem(nRow, nCol);

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgShaped::OnRButtonDown(UINT nFlags, CPoint point)
{
	int nRow = -1;
	int nCol = -1;
	if (HitTestCell(point, nRow, nCol))
	{
		// 右键清空方格，只重画这一格
		m_cells[nRow][nCol].strItem.Empty();
		m_cells[nRow][nCol].nCount = 0;
		InvalidateCell(nRow, nCol);
	}

	CDialogEx::OnRButtonDown(nFlags, point);
}

void CDlgShaped::OnMouseMove(UINT nFlags, CPoint point)
{
	int nRow = -1;
	int nCol = -1;
	if (!HitTestCell(point, nRow, nCol))
	{
		nRow = -1;
		nCol = -1;
	}

	if (nRow != m_nHoverRow || nCol != m_nHoverCol)
	{
		// 高亮变化时只重画受影响的两个方格
		if (m_nHoverRow >= 0 && m_nHoverCol >= 0)
			InvalidateCell(m_nHoverRow, m_nHoverCol);
		if (nRow >= 0 && nCol >= 0)
			InvalidateCell(nRow, nCol);

		m_nHoverRow = nRow;
		m_nHoverCol = nCol;
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

void CDlgShaped::OnMouseLeave()
{
	m_bTrackingMouse = FALSE;

	if (m_nHoverRow >= 0 || m_nHoverCol >= 0)
	{
		const int nRow = m_nHoverRow;
		const int nCol = m_nHoverCol;
		m_nHoverRow = -1;
		m_nHoverCol = -1;

		if (nRow >= 0 && nCol >= 0)
			InvalidateCell(nRow, nCol);
	}

	CDialogEx::OnMouseLeave();
}

BOOL CDlgShaped::SelectCellItem(int nRow, int nCol)
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
	const CraftCell& cell = m_cells[nRow][nCol];
	if (!cell.IsEmpty())
		dlgSelect.SetInitialSelection(cell.strItem, cell.nCount);

	if (dlgSelect.DoModal() != IDOK)
		return FALSE;

	m_cells[nRow][nCol].strItem = dlgSelect.GetSelectedItem();
	m_cells[nRow][nCol].nCount = dlgSelect.GetSelectedCount();
	InvalidateCell(nRow, nCol);

	return TRUE;
}
