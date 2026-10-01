// CDlgCraftable.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgCraftable.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDlgCraftable 对话框

IMPLEMENT_DYNAMIC(CDlgCraftable, CDialogEx)

CDlgCraftable::CDlgCraftable(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGCRAFTABLE, pParent)
	, m_rcHint(0, 0, 0, 0)
	, m_nHoverSlot(-1)
	, m_bTrackingMouse(FALSE)
	, m_pItemSource(nullptr)
	, m_pTextureStore(nullptr)
{

}

CDlgCraftable::~CDlgCraftable()
{
}

void CDlgCraftable::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CDlgCraftable::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}

void CDlgCraftable::SetTextureStore(CItemTextureStore* pTextureStore)
{
	m_pTextureStore = pTextureStore;
}

BEGIN_MESSAGE_MAP(CDlgCraftable, CDialogEx)
	ON_BN_CLICKED(IDC_RADIOSHAPED, &CDlgCraftable::OnRecipeTypeChanged)
	ON_BN_CLICKED(IDC_RADIOSHAPELESS, &CDlgCraftable::OnRecipeTypeChanged)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


// CDlgCraftable 消息处理程序

BOOL CDlgCraftable::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_ui.Initialize(this, 700, 440);
	// 资源中的单选框随代码绘制的网格一起缩放，放在产物格下方。
	m_ui.Move(IDC_RADIOSHAPED, 390, 268, 130, 28);
	m_ui.Move(IDC_RADIOSHAPELESS, 530, 268, 140, 28);
	GetDlgItem(IDC_RADIOSHAPED)->ModifyStyle(0, WS_GROUP | WS_TABSTOP);
	GetDlgItem(IDC_RADIOSHAPELESS)->ModifyStyle(WS_GROUP | WS_TABSTOP, 0);
	GetDlgItem(IDOK)->ModifyStyle(0, WS_GROUP);
	CheckRadioButton(IDC_RADIOSHAPED, IDC_RADIOSHAPELESS,
		m_bShapeless ? IDC_RADIOSHAPELESS : IDC_RADIOSHAPED);

	// 九宫格和输出格没有对应的资源控件，位置大小在这里一次性算好
	CalcSlotRects();


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDlgCraftable::OnOK()
{
	CString strError;
	if (!ValidateSlots(strError))
	{
		AfxMessageBox(strError, MB_ICONINFORMATION);
		return;
	}

	CDialogEx::OnOK();
}

LPCTSTR CDlgCraftable::GetHintText() const
{
	return m_bShapeless
		? _T("无序合成：材料摆在哪个格子都一样；原料总数不能超过 9 个。")
		: _T("有序合成：按九宫格中的材料摆放位置生成配方。");
}

void CDlgCraftable::OnRecipeTypeChanged()
{
	m_bShapeless = IsDlgButtonChecked(IDC_RADIOSHAPELESS) == BST_CHECKED;
	CheckRadioButton(IDC_RADIOSHAPED, IDC_RADIOSHAPELESS,
		m_bShapeless ? IDC_RADIOSHAPELESS : IDC_RADIOSHAPED);
	InvalidateRect(&m_rcHint, TRUE);
}

CString CDlgCraftable::BuildRecipeScript() const
{
	return m_bShapeless ? BuildShapelessScript() : BuildShapedScript();
}

BOOL CDlgCraftable::ValidateSlots(CString& strError) const
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

	if (m_bShapeless)
	{
		int total = 0;
		for (const auto& row : m_cells)
			for (const auto& cell : row)
				if (!cell.IsEmpty()) total += (std::max)(1, cell.nCount);
		if (total > GRID_CELLS)
		{
			strError.Format(_T("无序合成的材料最多 %d 个，现在有 %d 个。\n请减少材料或数量。"),
				GRID_CELLS, total);
			return FALSE;
		}
	}
	return TRUE;
}

void CDlgCraftable::CalcSlotRects()
{
	// 原料区与产物区的中心分别为 185、515，关于窗口中心 350 对称。
	for (int row = 0; row < GRID_ROWS; ++row)
		for (int col = 0; col < GRID_COLS; ++col)
			m_rcSlots[row * GRID_COLS + col] = m_ui.Rect(89 + col * 67, 110 + row * 67, 58, 58);
	m_rcSlots[SLOT_OUTPUT] = m_ui.Rect(480, 171, 70, 70);
	m_rcHint = m_ui.Rect(22, 350, 656, 42);
	for (int slot = 0; slot < SLOT_COUNT; ++slot) InvalidateSlot(slot);
}

CDlgCraftable::CraftCell* CDlgCraftable::GetSlot(int nSlot)
{
	// 复用常量版本，避免两处判断逻辑写得不一致
	return const_cast<CraftCell*>(static_cast<const CDlgCraftable*>(this)->GetSlot(nSlot));
}

const CDlgCraftable::CraftCell* CDlgCraftable::GetSlot(int nSlot) const
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return nullptr;
	if (nSlot == SLOT_OUTPUT)
		return &m_outputCell;

	return &m_cells[nSlot / GRID_COLS][nSlot % GRID_COLS];
}

BOOL CDlgCraftable::HitTestSlot(CPoint point, int& nSlot) const
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

void CDlgCraftable::InvalidateSlot(int nSlot)
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return;

	const auto* cell = GetSlot(nSlot);
	CString label;
	label.Format(nSlot == SLOT_OUTPUT ? _T("产物") : _T("原料 %d"), nSlot + 1);
	m_ui.Tip(nSlot + 1, m_rcSlots[nSlot], label, cell->strItem, cell->nCount);
	InvalidateRect(&m_rcSlots[nSlot], FALSE);
}

void CDlgCraftable::OnPaint()
{
	CPaintDC dc(this);
	DrawSlots(&dc);
}

void CDlgCraftable::DrawSlots(CDC* pDC)
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


void CDlgCraftable::OnLButtonDown(UINT nFlags, CPoint point)
{
	int nSlot = -1;
	if (HitTestSlot(point, nSlot))
		SelectSlotItem(nSlot);

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgCraftable::OnRButtonDown(UINT nFlags, CPoint point)
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

void CDlgCraftable::OnMouseMove(UINT nFlags, CPoint point)
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

void CDlgCraftable::OnMouseLeave()
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

BOOL CDlgCraftable::SelectSlotItem(int nSlot)
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

BOOL CDlgCraftable::PreTranslateMessage(MSG* message)
{
	m_ui.Relay(message);
	return CDialogEx::PreTranslateMessage(message);
}

CString CDlgCraftable::BuildShapedScript() const
{
	if (m_outputCell.IsEmpty())
		return CString();

	// 先找出九宫格里材料占用的范围：形状配方本身会自动对齐，
	// 把四周的空行空列去掉不影响结果，脚本看起来也干净
	int nTop = GRID_ROWS;
	int nBottom = -1;
	int nLeft = GRID_COLS;
	int nRight = -1;
	for (int nRow = 0; nRow < GRID_ROWS; ++nRow)
	{
		for (int nCol = 0; nCol < GRID_COLS; ++nCol)
		{
			if (m_cells[nRow][nCol].IsEmpty())
				continue;

			if (nRow < nTop)    nTop = nRow;
			if (nRow > nBottom) nBottom = nRow;
			if (nCol < nLeft)   nLeft = nCol;
			if (nCol > nRight)  nRight = nCol;
		}
	}

	if (nBottom < 0)	// 九宫格是空的
		return CString();

	// 同一种材料共用同一个字母，最多九种，正好 A ~ I
	CString arrItems[GRID_CELLS];
	int nItemCount = 0;
	CStringArray arrPattern;

	for (int nRow = nTop; nRow <= nBottom; ++nRow)
	{
		CString strRow;
		for (int nCol = nLeft; nCol <= nRight; ++nCol)
		{
			const CString& strItem = m_cells[nRow][nCol].strItem;
			if (strItem.IsEmpty())
			{
				strRow += _T(' ');
				continue;
			}

			int nIndex = -1;
			for (int k = 0; k < nItemCount; ++k)
			{
				if (arrItems[k] == strItem)
				{
					nIndex = k;
					break;
				}
			}
			if (nIndex < 0)
			{
				nIndex = nItemCount;
				arrItems[nItemCount++] = strItem;
			}

			strRow += (TCHAR)(_T('A') + nIndex);
		}
		arrPattern.Add(strRow);
	}

	// 产物：数量大于 1 时写成 Item.of(...)
	CString strOutput;
	if (m_outputCell.nCount > 1)
		strOutput.Format(_T("Item.of('%s', %d)"), m_outputCell.strItem.GetString(), m_outputCell.nCount);
	else
		strOutput.Format(_T("'%s'"), m_outputCell.strItem.GetString());

	// 形状
	CString strPattern;
	for (INT_PTR i = 0; i < arrPattern.GetSize(); ++i)
	{
		CString strLine;
		strLine.Format(_T("      '%s'"), arrPattern[i].GetString());
		if (i + 1 < arrPattern.GetSize())
			strLine += _T(',');
		strLine += _T("\r\n");
		strPattern += strLine;
	}

	// 材料表
	CString strKeys;
	for (int k = 0; k < nItemCount; ++k)
	{
		CString strLine;
		strLine.Format(_T("      %c: '%s'"), (TCHAR)(_T('A') + k), arrItems[k].GetString());
		if (k + 1 < nItemCount)
			strLine += _T(',');
		strLine += _T("\r\n");
		strKeys += strLine;
	}

	CString strScript;
	strScript.Format(
		_T("ServerEvents.recipes(event => {\r\n")
		_T("  // 有序合成 %s\r\n")
		_T("  event.shaped(\r\n")
		_T("    %s,\r\n")
		_T("    [\r\n")
		_T("%s")
		_T("    ],\r\n")
		_T("    {\r\n")
		_T("%s")
		_T("    }\r\n")
		_T("  )\r\n")
		_T("})\r\n"),
		m_outputCell.strItem.GetString(), strOutput.GetString(),
		strPattern.GetString(), strKeys.GetString());

	return strScript;
}

CString CDlgCraftable::BuildShapelessScript() const
{
	if (m_outputCell.IsEmpty())
		return CString();

	// 无序合成只认材料的种类和数量，所以按格子顺序把每种材料按数量摊平列出来
	CStringArray arrIngredients;
	for (int nRow = 0; nRow < GRID_ROWS; ++nRow)
	{
		for (int nCol = 0; nCol < GRID_COLS; ++nCol)
		{
			const CraftCell& cell = m_cells[nRow][nCol];
			if (cell.IsEmpty())
				continue;

			const int nCount = (cell.nCount > 1) ? cell.nCount : 1;
			for (int k = 0; k < nCount; ++k)
				arrIngredients.Add(cell.strItem);
		}
	}

	if (arrIngredients.GetSize() == 0)	// 九宫格是空的
		return CString();

	// 产物：数量大于 1 时写成 Item.of(...)
	CString strOutput;
	if (m_outputCell.nCount > 1)
		strOutput.Format(_T("Item.of('%s', %d)"), m_outputCell.strItem.GetString(), m_outputCell.nCount);
	else
		strOutput.Format(_T("'%s'"), m_outputCell.strItem.GetString());

	// 材料清单
	CString strList;
	for (INT_PTR i = 0; i < arrIngredients.GetSize(); ++i)
	{
		CString strLine;
		strLine.Format(_T("      '%s'"), arrIngredients[i].GetString());
		if (i + 1 < arrIngredients.GetSize())
			strLine += _T(',');
		strLine += _T("\r\n");
		strList += strLine;
	}

	CString strScript;
	strScript.Format(
		_T("ServerEvents.recipes(event => {\r\n")
		_T("  // 无序合成 %s\r\n")
		_T("  event.shapeless(\r\n")
		_T("    %s,\r\n")
		_T("    [\r\n")
		_T("%s")
		_T("    ]\r\n")
		_T("  )\r\n")
		_T("})\r\n"),
		m_outputCell.strItem.GetString(), strOutput.GetString(), strList.GetString());

	return strScript;
}
