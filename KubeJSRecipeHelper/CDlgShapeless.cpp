// CDlgShapeless.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgShapeless.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDlgShapeless 对话框

IMPLEMENT_DYNAMIC(CDlgShapeless, CDlgCraftBase)

CDlgShapeless::CDlgShapeless(CWnd* pParent /*=nullptr*/)
	: CDlgCraftBase(IDD_DLGSHAPELESS, pParent)
{

}

CDlgShapeless::~CDlgShapeless()
{
}

BEGIN_MESSAGE_MAP(CDlgShapeless, CDlgCraftBase)
END_MESSAGE_MAP()


// CDlgShapeless 消息处理程序

LPCTSTR CDlgShapeless::GetHintText() const
{
	return _T("无序合成：材料摆在哪个格子都一样；点“确定”生成脚本");
}

BOOL CDlgShapeless::ValidateSlots(CString& strError) const
{
	if (!CDlgCraftBase::ValidateSlots(strError))
		return FALSE;

	// 无序合成把每种材料按数量摊平列出来，总数还是受工作台九格的限制
	int nTotal = 0;
	for (int nRow = 0; nRow < GRID_ROWS; ++nRow)
	{
		for (int nCol = 0; nCol < GRID_COLS; ++nCol)
		{
			const CraftCell& cell = m_cells[nRow][nCol];
			if (!cell.IsEmpty())
				nTotal += (cell.nCount > 1) ? cell.nCount : 1;
		}
	}

	if (nTotal > GRID_CELLS)
	{
		strError.Format(_T("无序合成的材料最多 %d 个，现在有 %d 个。\n")
			_T("同一种材料多放几份，或者把数量调小一点。"), GRID_CELLS, nTotal);
		return FALSE;
	}

	return TRUE;
}

CString CDlgShapeless::BuildRecipeScript() const
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
