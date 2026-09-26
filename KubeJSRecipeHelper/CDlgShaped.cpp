// CDlgShaped.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgShaped.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDlgShaped 对话框

IMPLEMENT_DYNAMIC(CDlgShaped, CDlgCraftBase)

CDlgShaped::CDlgShaped(CWnd* pParent /*=nullptr*/)
	: CDlgCraftBase(IDD_DLGSHAPED, pParent)
{

}

CDlgShaped::~CDlgShaped()
{
}

BEGIN_MESSAGE_MAP(CDlgShaped, CDlgCraftBase)
END_MESSAGE_MAP()


// CDlgShaped 消息处理程序

CString CDlgShaped::BuildRecipeScript() const
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
