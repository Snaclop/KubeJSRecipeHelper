// CDlgSmith.cpp: 实现文件
//

#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgSmith.h"
#include "CDlgSelect.h"


// CDlgSmith 对话框

IMPLEMENT_DYNAMIC(CDlgSmith, CDialogEx)

CDlgSmith::CDlgSmith(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGSMITH, pParent)
	, m_pItemSource(nullptr)
{
	// 仿照 MC 物品栏格子的灰色
	m_brSlot.CreateSolidBrush(RGB(0x8B, 0x8B, 0x8B));
}

CDlgSmith::~CDlgSmith()
{
}

void CDlgSmith::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CDlgSmith::SetItemSource(const CStringArray* pItemSource)
{
	m_pItemSource = pItemSource;
}


BEGIN_MESSAGE_MAP(CDlgSmith, CDialogEx)
	ON_WM_CTLCOLOR()
	ON_STN_CLICKED(IDC_STATICTEMPLATE, &CDlgSmith::OnStnClickedStaticTemplate)
	ON_STN_CLICKED(IDC_STATICUPGRADE, &CDlgSmith::OnStnClickedStaticUpgrade)
	ON_STN_CLICKED(IDC_STATICINGREDIENT, &CDlgSmith::OnStnClickedStaticIngredient)
	ON_STN_CLICKED(IDC_STATICOUTPUT, &CDlgSmith::OnStnClickedStaticOutput)
END_MESSAGE_MAP()


// CDlgSmith 消息处理程序

UINT CDlgSmith::GetSlotCtrlID(int nSlot) const
{
	switch (nSlot)
	{
	case SLOT_TEMPLATE:   return IDC_STATICTEMPLATE;
	case SLOT_UPGRADE:    return IDC_STATICUPGRADE;
	case SLOT_INGREDIENT: return IDC_STATICINGREDIENT;
	case SLOT_OUTPUT:     return IDC_STATICOUTPUT;
	default:              return 0;
	}
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

	// 槽位里已经有物品时，带着原来的物品和数量打开，方便修改
	if (nSlot >= 0 && nSlot < SLOT_COUNT && !m_slots[nSlot].strItem.IsEmpty())
		dlgSelect.SetInitialSelection(m_slots[nSlot].strItem, m_slots[nSlot].nCount);

	if (dlgSelect.DoModal() != IDOK)
		return FALSE;

	if (nSlot >= 0 && nSlot < SLOT_COUNT)
	{
		m_slots[nSlot].strItem = dlgSelect.GetSelectedItem();
		m_slots[nSlot].nCount = dlgSelect.GetSelectedCount();
		UpdateSlotText(nSlot);
	}

	return TRUE;
}

void CDlgSmith::UpdateSlotText(int nSlot)
{
	if (nSlot < 0 || nSlot >= SLOT_COUNT)
		return;

	CWnd* pCtrl = GetDlgItem(GetSlotCtrlID(nSlot));
	if (pCtrl == nullptr)
		return;

	CString strText;
	const SmithSlot& slot = m_slots[nSlot];
	if (!slot.strItem.IsEmpty())
	{
		// 物品 id 一般带命名空间，矩形比较窄，只显示物品名与数量
		CString strName(slot.strItem);
		const int nColon = strName.Find(_T(':'));
		if (nColon >= 0)
			strName = strName.Mid(nColon + 1);

		if (slot.nCount > 1)
			strText.Format(_T("%s x%d"), strName.GetString(), slot.nCount);
		else
			strText = strName;
	}

	pCtrl->SetWindowText(strText);
}

void CDlgSmith::OnStnClickedStaticTemplate()
{
	SelectSlotItem(SLOT_TEMPLATE);
}

void CDlgSmith::OnStnClickedStaticUpgrade()
{
	SelectSlotItem(SLOT_UPGRADE);
}

void CDlgSmith::OnStnClickedStaticIngredient()
{
	SelectSlotItem(SLOT_INGREDIENT);
}

void CDlgSmith::OnStnClickedStaticOutput()
{
	SelectSlotItem(SLOT_OUTPUT);
}

HBRUSH CDlgSmith::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	// 四个矩形刷成灰色背景，保持原来 SS_GRAYRECT 的观感；文字正常绘制
	if (nCtlColor == CTLCOLOR_STATIC && pWnd != nullptr)
	{
		const UINT nID = (UINT)pWnd->GetDlgCtrlID();
		if (nID == IDC_STATICTEMPLATE || nID == IDC_STATICUPGRADE ||
			nID == IDC_STATICINGREDIENT || nID == IDC_STATICOUTPUT)
		{
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(RGB(0xFF, 0xFF, 0xFF));
			return (HBRUSH)m_brSlot.GetSafeHandle();
		}
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CDlgSmith::OnOK()
{
	// 四个槽位都选好才能生成脚本
	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		if (m_slots[i].strItem.IsEmpty())
		{
			AfxMessageBox(_T("请先点击四个矩形，把模板、升级物品、材料和产物都选好。"), MB_ICONINFORMATION);
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
	const SmithSlot& out = m_slots[SLOT_OUTPUT];
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
