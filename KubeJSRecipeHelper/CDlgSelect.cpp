#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgSelect.h"
#include "Language.h"
#include "NumberText.h"
#include <cmath>
#include <cwchar>

IMPLEMENT_DYNAMIC(CDlgSelect, CDialogEx)
CDlgSelect::CDlgSelect(CWnd* pParent) : CDialogEx(IDD_DLGSELECT, pParent), m_pItemList(nullptr),
    m_nInitialCount(1), m_nSelectedCount(1), m_nMaxCount(COUNT_MAX) {}
CDlgSelect::~CDlgSelect() {}
void CDlgSelect::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST, m_listItems); DDX_Control(pDX, IDC_EDITSEARCH, m_editSearch);
    DDX_Control(pDX, IDC_EDITNUMBER, m_editNumber); DDX_Control(pDX, IDC_SPINNUMBER, m_spinNumber);
    DDX_Control(pDX, IDC_EDITPOSSIBILITY, m_editChance); DDX_Control(pDX, IDC_SPINPOSSIBILITY, m_spinChance);
    DDX_Control(pDX, IDC_COMBOITEMTAG, m_comboItemTag);
}
void CDlgSelect::SetItemList(const CStringArray* pItemList) { m_pItemList = pItemList; }
void CDlgSelect::SetInitialSelection(const CString& strId, int nCount) { m_strInitialItem = strId; m_nInitialCount = nCount; }
void CDlgSelect::SetCountLimit(int nCount, bool bForFluids) {
    (bForFluids ? m_nMaxFluidCount : m_nMaxCount) = max(1, min(COUNT_MAX, nCount));
}
int CDlgSelect::CurrentLimit() const { return m_eKind == RecipeFluid ? m_nMaxFluidCount : m_nMaxCount; }
BOOL CDlgSelect::OnInitDialog() {
    CDialogEx::OnInitDialog();
    UiLanguage::ApplyDialog(*this);
    m_bTagMode = m_bAllowTags && RecipeIsTag(std::wstring(m_strInitialItem.GetString()));
    m_arrPreferred[m_bTagMode ? 1 : 0] = m_strInitialItem;
    const wchar_t* pszConcrete = m_uAllowedKinds == RecipeFluid ? L"流体" : m_uAllowedKinds == RecipeItem ? L"物品" : L"物品 / 流体";
    int nIndex = m_comboItemTag.AddString(UiLanguage::Text(pszConcrete)); m_comboItemTag.SetItemData(nIndex, 0);
    if (m_bAllowTags) { nIndex = m_comboItemTag.AddString(UiLanguage::Text(L"标签")); m_comboItemTag.SetItemData(nIndex, 1); }
    for (int nIndex = 0; nIndex < m_comboItemTag.GetCount(); ++nIndex) if (m_comboItemTag.GetItemData(nIndex) == (m_bTagMode ? 1 : 0)) m_comboItemTag.SetCurSel(nIndex);
    m_comboItemTag.EnableWindow(m_bAllowTags);
    m_editSearch.SendMessage(EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(UiLanguage::Text(m_bTagMode ? L"搜索或填写标签 ID" : L"搜索物品 ID")));
    m_spinNumber.ModifyStyle(0, UDS_NOTHOUSANDS);
    m_spinNumber.SetRange32(1, CurrentLimit()); m_spinNumber.SetPos32(max(1, min(CurrentLimit(), m_nInitialCount)));
    // Decimal chance/weight editing must not be overwritten by an integer buddy.
    m_spinChance.ModifyStyle(UDS_SETBUDDYINT, 0); m_spinChance.SetRange32(0, 1000000);
    CString strValue(FormatRecipeNumber(m_dChance).c_str()); m_editChance.SetWindowText(strValue);
    GetDlgItem(IDC_GROUPCHANCE)->SetWindowText(m_bWeights ? UiLanguage::Text(L"权重") : UiLanguage::Text(L"概率%"));
    RefreshList(m_strInitialItem); OnSelectionChanged(); return TRUE;
}
void CDlgSelect::RefreshList(const CString& strPrefer) {
    m_listItems.ResetContent(); m_arrChoices.clear();
    CString strSearch; m_editSearch.GetWindowText(strSearch); strSearch.MakeLower();
    CStringArray arrKeywords; int nPosition = 0;
    CString strWord = strSearch.Tokenize(_T(" \t"), nPosition);
    while (!strWord.IsEmpty()) { arrKeywords.Add(strWord); strWord = strSearch.Tokenize(_T(" \t"), nPosition); }
    int nSelectedIndex = LB_ERR; CString strWanted = strPrefer;
    auto add = [&](const CString& strId, unsigned uKinds) {
        CString strLower(strId); strLower.MakeLower(); bool bMatch = true;
        for (INT_PTR nKeywordIndex = 0; nKeywordIndex < arrKeywords.GetSize(); ++nKeywordIndex) if (strLower.Find(arrKeywords[nKeywordIndex]) < 0) bMatch = false;
        if (!bMatch) return;
        uKinds &= m_uAllowedKinds;
        for (auto eKind : { RecipeItem, RecipeFluid }) {
            if (!(uKinds & eKind)) continue;
            CString strDisplay = strId;
            if (eKind == RecipeFluid) strDisplay += UiLanguage::Text(L"  [流体]");
            if (m_listItems.FindStringExact(-1, strDisplay) != LB_ERR) continue;
            m_arrChoices.push_back({ strId, eKind });
            int nIndex = m_listItems.AddString(strDisplay);
            if (nIndex >= 0) m_listItems.SetItemData(nIndex, m_arrChoices.size());
        }
    };
    if (m_bTagMode) {
        if (m_pCatalog) for (const auto& entry : *m_pCatalog)
            if (RecipeIsTag(entry.first)) add(entry.first.c_str(), entry.second);
        CString strManual; m_editSearch.GetWindowText(strManual); strManual.Trim();
        if (!strManual.IsEmpty() && strManual[0] != L'#') strManual = L"#" + strManual;
        if (RecipeValidId(std::wstring(strManual.GetString()), true)) { add(strManual, m_uAllowedKinds); strWanted = strManual; }
        // Keep a manually entered tag selectable when reopening its slot.
        if (RecipeIsTag(std::wstring(strPrefer.GetString())) && RecipeValidId(std::wstring(strPrefer.GetString()), true)) add(strPrefer, m_eKind);
    } else if (m_pItemList) for (INT_PTR nIndex = 0; nIndex < m_pItemList->GetSize(); ++nIndex) {
        const auto& strId = m_pItemList->GetAt(nIndex);
        if (!RecipeIsTag(std::wstring(strId.GetString()))) add(strId, RecipeKinds(m_pCatalog, std::wstring(strId.GetString())));
    }
    if (m_arrChoices.empty()) {
        int nIndex = m_listItems.AddString(UiLanguage::Text(m_bTagMode ? L"（可直接填写完整标签 ID，例如 #minecraft:saplings）" : L"（没有匹配的项目，请导入相应 jar 或修改搜索词）"));
        if (nIndex >= 0) m_listItems.SetItemData(nIndex, 0);
    } else {
        for (int nIndex = 0; nIndex < m_listItems.GetCount(); ++nIndex) {
            auto uData = m_listItems.GetItemData(nIndex);
            if (uData && uData <= m_arrChoices.size() && m_arrChoices[uData - 1].strId == strWanted && m_arrChoices[uData - 1].eKind == m_eKind) nSelectedIndex = nIndex;
        }
        m_listItems.SetCurSel(nSelectedIndex == LB_ERR ? 0 : nSelectedIndex);
    }
}
BOOL CDlgSelect::GetCurSelItem(CString& strId) const {
    int nIndex = m_listItems.GetCurSel(); if (nIndex < 0) return FALSE;
    auto uData = m_listItems.GetItemData(nIndex); if (!uData || uData > m_arrChoices.size()) return FALSE;
    strId = m_arrChoices[uData - 1].strId; return TRUE;
}
void CDlgSelect::OnSelectionChanged() {
    int nIndex = m_listItems.GetCurSel();
    if (nIndex >= 0) { auto uData = m_listItems.GetItemData(nIndex); if (uData && uData <= m_arrChoices.size()) m_eKind = m_arrChoices[uData - 1].eKind; }
    BOOL bValid; UINT uCount = GetDlgItemInt(IDC_EDITNUMBER, &bValid, FALSE);
    int nLimit = CurrentLimit();
    m_spinNumber.SetRange32(1, nLimit);
    m_spinNumber.SetPos32(bValid ? min((int)min(uCount, (UINT)COUNT_MAX), nLimit) : 1);
    m_editNumber.EnableWindow(nLimit != 1); m_spinNumber.EnableWindow(nLimit != 1);
    GetDlgItem(IDC_GROUPCOUNT)->SetWindowText(m_eKind == RecipeFluid ? UiLanguage::Text(L"体积(mB)") : UiLanguage::Text(L"数量"));
    BOOL bChanceEnabled = m_bAllowChance && m_eKind == RecipeItem && !m_bTagMode;
    m_editChance.EnableWindow(bChanceEnabled); m_spinChance.EnableWindow(bChanceEnabled);
}
void CDlgSelect::OnOK() {
    CString strId; if (!GetCurSelItem(strId)) { AfxMessageBox(UiLanguage::Text(L"请先选择一个项目。")); return; }
    OnSelectionChanged();
    BOOL bValid; UINT uCount = GetDlgItemInt(IDC_EDITNUMBER, &bValid, FALSE);
    m_nSelectedCount = bValid ? max(1, min((int)min(uCount, (UINT)COUNT_MAX), CurrentLimit())) : 1;
    if (m_editChance.IsWindowEnabled()) {
        CString strText; m_editChance.GetWindowText(strText); strText.Trim(); wchar_t* pEnd = nullptr;
        double dChance = wcstod(strText.GetString(), &pEnd);
        if (pEnd == strText.GetString() || *pEnd || !std::isfinite(dChance) ||
            (m_bWeights ? dChance <= 0 || dChance > 1000000 : dChance < 0 || dChance > 100)) {
            AfxMessageBox(m_bWeights ? UiLanguage::Text(L"权重须大于 0 且不超过 1000000。") : UiLanguage::Text(L"概率须为 0–100%，支持小数。")); return;
        }
        m_dChance = dChance;
    } else m_dChance = 100;
    m_strSelectedItem = strId; CDialogEx::OnOK();
}
void CDlgSelect::OnChanceSpin(NMHDR* pHeader, LRESULT* pResult) {
    auto pDelta = reinterpret_cast<NMUPDOWN*>(pHeader); CString strText; m_editChance.GetWindowText(strText);
    double dValue = wcstod(strText, nullptr) - pDelta->iDelta;
    dValue = max(m_bWeights ? 0.01 : 0.0, min(m_bWeights ? 1000000.0 : 100.0, dValue));
    strText = FormatRecipeNumber(dValue).c_str(); m_editChance.SetWindowText(strText); *pResult = 1;
}
BOOL CDlgSelect::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->hwnd == m_editSearch.GetSafeHwnd()) {
        m_listItems.SetFocus(); return TRUE;
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}
void CDlgSelect::OnEnChangeEditSearch() {
    CString strCurrent; GetCurSelItem(strCurrent); RefreshList(strCurrent); OnSelectionChanged();
}
void CDlgSelect::OnLbnDblclkList() { OnOK(); }
void CDlgSelect::OnItemTagChanged() {
    CString strCurrent; GetCurSelItem(strCurrent); m_arrPreferred[m_bTagMode ? 1 : 0] = strCurrent;
    const int nSelectedIndex = m_comboItemTag.GetCurSel(); if (nSelectedIndex == CB_ERR) return;
    m_bTagMode = m_bAllowTags && m_comboItemTag.GetItemData(nSelectedIndex) == 1;
    m_editSearch.SetWindowText(L"");
    m_editSearch.SendMessage(EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(UiLanguage::Text(m_bTagMode ? L"搜索或填写标签 ID" : L"搜索物品 ID")));
    RefreshList(m_arrPreferred[m_bTagMode ? 1 : 0]); OnSelectionChanged();
}
BEGIN_MESSAGE_MAP(CDlgSelect, CDialogEx)
    ON_CBN_SELCHANGE(IDC_COMBOITEMTAG, &CDlgSelect::OnItemTagChanged)
    ON_EN_CHANGE(IDC_EDITSEARCH, &CDlgSelect::OnEnChangeEditSearch)
    ON_LBN_DBLCLK(IDC_LIST, &CDlgSelect::OnLbnDblclkList)
    ON_LBN_SELCHANGE(IDC_LIST, &CDlgSelect::OnSelectionChanged)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPINPOSSIBILITY, &CDlgSelect::OnChanceSpin)
END_MESSAGE_MAP()
