// CDlgSelect.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	// 列表项的数据：用来区分真正的物品与“没有可选物品”时的占位提示
	const DWORD_PTR LISTDATA_REAL_ITEM = 1;
	const DWORD_PTR LISTDATA_HINT      = 0;

	// 搜索关键字的分隔符
	BOOL IsSearchSeparator(TCHAR ch)
	{
		return ch == _T(' ') || ch == _T('\t');
	}

	// 物品 id 是否命中全部关键字（忽略大小写）
	BOOL MatchKeywords(const CString& strItem, const CStringArray& arrKeywords)
	{
		if (arrKeywords.GetSize() == 0)
			return TRUE;

		CString strLower(strItem);
		strLower.MakeLower();

		for (INT_PTR i = 0; i < arrKeywords.GetSize(); ++i)
		{
			if (strLower.Find(arrKeywords[i]) < 0)
				return FALSE;
		}

		return TRUE;
	}
}


// CDlgSelect 对话框

IMPLEMENT_DYNAMIC(CDlgSelect, CDialogEx)

CDlgSelect::CDlgSelect(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGSELECT, pParent)
	, m_pItemList(nullptr)
	, m_nInitialCount(1)
	, m_nSelectedCount(1)
{

}

CDlgSelect::~CDlgSelect()
{
}

void CDlgSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_List);
	DDX_Control(pDX, IDC_EDITSEARCH, m_EditSearch);
	DDX_Control(pDX, IDC_EDITNUMBER, m_EditNumber);
	DDX_Control(pDX, IDC_SPIN1, m_Spin);
}

void CDlgSelect::SetItemList(const CStringArray* pItemList)
{
	m_pItemList = pItemList;
}

void CDlgSelect::SetInitialSelection(const CString& strItem, int nCount)
{
	m_strInitialItem = strItem;
	m_nInitialCount = nCount;
}

BOOL CDlgSelect::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 数量的初始值先夹到合法范围内
	int nCount = m_nInitialCount;
	if (nCount < COUNT_MIN)
		nCount = COUNT_MIN;
	if (nCount > COUNT_MAX)
		nCount = COUNT_MAX;

	// 数量框交给微调按钮控制，范围就是 MC 单格的堆叠上限
	m_Spin.SetRange32(COUNT_MIN, COUNT_MAX);
	m_Spin.SetBuddy(&m_EditNumber);
	m_Spin.SetPos(nCount);

	CString strCount;
	strCount.Format(_T("%d"), nCount);
	m_EditNumber.SetWindowText(strCount);

	// 列表按搜索框的内容填充，并预先选中传进来的物品
	RefreshList(m_strInitialItem);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDlgSelect::RefreshList(const CString& strPrefer)
{
	m_List.ResetContent();

	// 搜索框里的内容按空格（或制表符）拆成若干关键字，全部命中才显示
	CStringArray arrKeywords;
	CString strSearch;
	m_EditSearch.GetWindowText(strSearch);

	int nPos = 0;
	while (nPos < strSearch.GetLength())
	{
		while (nPos < strSearch.GetLength() && IsSearchSeparator(strSearch[nPos]))
			++nPos;

		const int nStart = nPos;
		while (nPos < strSearch.GetLength() && !IsSearchSeparator(strSearch[nPos]))
			++nPos;

		if (nPos > nStart)
		{
			CString strKeyword = strSearch.Mid(nStart, nPos - nStart);
			strKeyword.MakeLower();
			arrKeywords.Add(strKeyword);
		}
	}

	if (m_pItemList != nullptr)
	{
		for (INT_PTR i = 0; i < m_pItemList->GetSize(); ++i)
		{
			const CString& strItem = m_pItemList->GetAt(i);
			if (!MatchKeywords(strItem, arrKeywords))
				continue;

			// 列表本身是排序的，同一个物品只显示一次
			if (m_List.FindStringExact(-1, strItem) != LB_ERR)
				continue;

			const int nIndex = m_List.AddString(strItem);
			if (nIndex != LB_ERR)
				m_List.SetItemData(nIndex, LISTDATA_REAL_ITEM);
		}
	}

	if (m_List.GetCount() == 0)
	{
		// 一个可选项都没有时给一行提示，免得界面看着像坏了
		const CString strHint = (m_pItemList == nullptr || m_pItemList->GetSize() == 0)
			? _T("（没有可选的物品，请先在主界面导入 jar 文件）")
			: _T("（没有匹配的物品）");

		const int nIndex = m_List.AddString(strHint);
		if (nIndex != LB_ERR)
			m_List.SetItemData(nIndex, LISTDATA_HINT);

		return;
	}

	// 尽量保持刷新前的选中项，其次是调用方指定的物品，最后退回第一项
	int nSel = strPrefer.IsEmpty() ? LB_ERR : m_List.FindStringExact(-1, strPrefer);
	if (nSel == LB_ERR)
		nSel = 0;
	m_List.SetCurSel(nSel);
}

BOOL CDlgSelect::GetCurSelItem(CString& strItem) const
{
	const int nSel = m_List.GetCurSel();
	if (nSel == LB_ERR || m_List.GetItemData(nSel) != LISTDATA_REAL_ITEM)
		return FALSE;

	m_List.GetText(nSel, strItem);
	return TRUE;
}

void CDlgSelect::OnOK()
{
	CString strItem;
	if (!GetCurSelItem(strItem))
	{
		AfxMessageBox(_T("请先在列表中选择一个物品。"), MB_ICONINFORMATION);
		return;
	}

	// 数量填得不对时按 1 处理，再夹到合法范围内
	BOOL bValid = FALSE;
	int nCount = (int)GetDlgItemInt(IDC_EDITNUMBER, &bValid, FALSE);
	if (!bValid || nCount < COUNT_MIN)
		nCount = COUNT_MIN;
	if (nCount > COUNT_MAX)
		nCount = COUNT_MAX;

	m_strSelectedItem = strItem;
	m_nSelectedCount = nCount;

	CDialogEx::OnOK();
}

BOOL CDlgSelect::PreTranslateMessage(MSG* pMsg)
{
	// 在搜索框里按回车时把焦点交给列表，而不是直接触发“确定”把对话框关掉
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
		pMsg->hwnd == m_EditSearch.GetSafeHwnd())
	{
		if (m_List.GetCount() > 0)
		{
			m_List.SetCurSel(0);
			m_List.SetFocus();
		}
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CDlgSelect, CDialogEx)
	ON_EN_CHANGE(IDC_EDITSEARCH, &CDlgSelect::OnEnChangeEditSearch)
	ON_LBN_DBLCLK(IDC_LIST, &CDlgSelect::OnLbnDblclkList)
END_MESSAGE_MAP()


// CDlgSelect 消息处理程序

void CDlgSelect::OnEnChangeEditSearch()
{
	// 搜索内容变了：尽量保留当前选中的物品
	CString strCur;
	GetCurSelItem(strCur);
	RefreshList(strCur);
}

void CDlgSelect::OnLbnDblclkList()
{
	// 双击列表项直接确认
	OnOK();
}
