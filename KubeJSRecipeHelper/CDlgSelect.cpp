#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgSelect.h"
#include "Language.h"
#include <cmath>
#include <cwchar>

IMPLEMENT_DYNAMIC(CDlgSelect, CDialogEx)
CDlgSelect::CDlgSelect(CWnd* parent) : CDialogEx(IDD_DLGSELECT, parent), m_pItemList(nullptr),
    m_nInitialCount(1), m_nSelectedCount(1), m_nMaxCount(COUNT_MAX) {}
CDlgSelect::~CDlgSelect() {}
void CDlgSelect::DoDataExchange(CDataExchange* dx) {
    CDialogEx::DoDataExchange(dx);
    DDX_Control(dx, IDC_LIST, m_List); DDX_Control(dx, IDC_EDITSEARCH, m_EditSearch);
    DDX_Control(dx, IDC_EDITNUMBER, m_EditNumber); DDX_Control(dx, IDC_SPINNUMBER, m_SpinNumber);
    DDX_Control(dx, IDC_EDITPOSSIBILITY, m_EditPossibility); DDX_Control(dx, IDC_SPINPOSSIBILITY, m_SpinPossibility);
    DDX_Control(dx, IDC_COMBOITEMTAG, m_ItemTag);
}
void CDlgSelect::SetItemList(const CStringArray* list) { m_pItemList = list; }
void CDlgSelect::SetInitialSelection(const CString& id, int count) { m_strInitialItem = id; m_nInitialCount = count; }
void CDlgSelect::SetCountLimit(int count, bool forFluids) {
    (forFluids ? m_nMaxFluidCount : m_nMaxCount) = max(1, min(COUNT_MAX, count));
}
int CDlgSelect::CurrentLimit() const { return m_kind == RecipeFluid ? m_nMaxFluidCount : m_nMaxCount; }
BOOL CDlgSelect::OnInitDialog() {
    CDialogEx::OnInitDialog();
    UiLanguage::ApplyDialog(*this);
    m_tagMode = m_allowTags && RecipeIsTag(std::wstring(m_strInitialItem.GetString()));
    m_preferred[m_tagMode ? 1 : 0] = m_strInitialItem;
    const wchar_t* concrete = m_allowedKinds == RecipeFluid ? L"流体" : m_allowedKinds == RecipeItem ? L"物品" : L"物品 / 流体";
    int index = m_ItemTag.AddString(UiLanguage::Text(concrete)); m_ItemTag.SetItemData(index, 0);
    if (m_allowTags) { index = m_ItemTag.AddString(UiLanguage::Text(L"标签")); m_ItemTag.SetItemData(index, 1); }
    for (int i = 0; i < m_ItemTag.GetCount(); ++i) if (m_ItemTag.GetItemData(i) == (m_tagMode ? 1 : 0)) m_ItemTag.SetCurSel(i);
    m_ItemTag.EnableWindow(m_allowTags);
    m_EditSearch.SendMessage(EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(UiLanguage::Text(m_tagMode ? L"搜索或填写标签 ID" : L"搜索物品 ID")));
    m_SpinNumber.ModifyStyle(0, UDS_NOTHOUSANDS);
    m_SpinNumber.SetRange32(1, CurrentLimit()); m_SpinNumber.SetPos32(max(1, min(CurrentLimit(), m_nInitialCount)));
    // Decimal chance/weight editing must not be overwritten by an integer buddy.
    m_SpinPossibility.ModifyStyle(UDS_SETBUDDYINT, 0); m_SpinPossibility.SetRange32(0, 1000000);
    CString value; value.Format(_T("%.12g"), m_chance); m_EditPossibility.SetWindowText(value);
    GetDlgItem(IDC_GROUPCHANCE)->SetWindowText(m_weights ? UiLanguage::Text(L"权重") : UiLanguage::Text(L"概率%"));
    RefreshList(m_strInitialItem); OnSelectionChanged(); return TRUE;
}
void CDlgSelect::RefreshList(const CString& prefer) {
    m_List.ResetContent(); m_choices.clear();
    CString search; m_EditSearch.GetWindowText(search); search.MakeLower();
    CStringArray keywords; int pos = 0;
    CString word = search.Tokenize(_T(" \t"), pos);
    while (!word.IsEmpty()) { keywords.Add(word); word = search.Tokenize(_T(" \t"), pos); }
    int selected = LB_ERR; CString wanted = prefer;
    auto add = [&](const CString& id, unsigned kinds) {
        CString lower(id); lower.MakeLower(); bool match = true;
        for (INT_PTR k = 0; k < keywords.GetSize(); ++k) if (lower.Find(keywords[k]) < 0) match = false;
        if (!match) return;
        kinds &= m_allowedKinds;
        for (auto kind : { RecipeItem, RecipeFluid }) {
            if (!(kinds & kind)) continue;
            CString display = id;
            if (kind == RecipeFluid) display += UiLanguage::Text(L"  [流体]");
            if (m_List.FindStringExact(-1, display) != LB_ERR) continue;
            m_choices.push_back({ id, kind });
            int index = m_List.AddString(display);
            if (index >= 0) m_List.SetItemData(index, m_choices.size());
        }
    };
    if (m_tagMode) {
        if (m_catalog) for (const auto& entry : *m_catalog)
            if (RecipeIsTag(entry.first)) add(entry.first.c_str(), entry.second);
        CString manual; m_EditSearch.GetWindowText(manual); manual.Trim();
        if (!manual.IsEmpty() && manual[0] != L'#') manual = L"#" + manual;
        if (RecipeValidId(std::wstring(manual.GetString()), true)) { add(manual, m_allowedKinds); wanted = manual; }
        // Keep a manually entered tag selectable when reopening its slot.
        if (RecipeIsTag(std::wstring(prefer.GetString())) && RecipeValidId(std::wstring(prefer.GetString()), true)) add(prefer, m_kind);
    } else if (m_pItemList) for (INT_PTR i = 0; i < m_pItemList->GetSize(); ++i) {
        const auto& id = m_pItemList->GetAt(i);
        if (!RecipeIsTag(std::wstring(id.GetString()))) add(id, RecipeKinds(m_catalog, std::wstring(id.GetString())));
    }
    if (m_choices.empty()) {
        int index = m_List.AddString(UiLanguage::Text(m_tagMode ? L"（可直接填写完整标签 ID，例如 #minecraft:saplings）" : L"（没有匹配的项目，请导入相应 jar 或修改搜索词）"));
        if (index >= 0) m_List.SetItemData(index, 0);
    } else {
        for (int i = 0; i < m_List.GetCount(); ++i) {
            auto data = m_List.GetItemData(i);
            if (data && data <= m_choices.size() && m_choices[data - 1].id == wanted && m_choices[data - 1].kind == m_kind) selected = i;
        }
        m_List.SetCurSel(selected == LB_ERR ? 0 : selected);
    }
}
BOOL CDlgSelect::GetCurSelItem(CString& id) const {
    int index = m_List.GetCurSel(); if (index < 0) return FALSE;
    auto data = m_List.GetItemData(index); if (!data || data > m_choices.size()) return FALSE;
    id = m_choices[data - 1].id; return TRUE;
}
void CDlgSelect::OnSelectionChanged() {
    int index = m_List.GetCurSel();
    if (index >= 0) { auto data = m_List.GetItemData(index); if (data && data <= m_choices.size()) m_kind = m_choices[data - 1].kind; }
    BOOL valid; UINT count = GetDlgItemInt(IDC_EDITNUMBER, &valid, FALSE);
    int limit = CurrentLimit();
    m_SpinNumber.SetRange32(1, limit);
    m_SpinNumber.SetPos32(valid ? min((int)min(count, (UINT)COUNT_MAX), limit) : 1);
    m_EditNumber.EnableWindow(limit != 1); m_SpinNumber.EnableWindow(limit != 1);
    GetDlgItem(IDC_GROUPCOUNT)->SetWindowText(m_kind == RecipeFluid ? UiLanguage::Text(L"体积(mB)") : UiLanguage::Text(L"数量"));
    BOOL chance = m_allowChance && m_kind == RecipeItem && !m_tagMode;
    m_EditPossibility.EnableWindow(chance); m_SpinPossibility.EnableWindow(chance);
}
void CDlgSelect::OnOK() {
    CString id; if (!GetCurSelItem(id)) { AfxMessageBox(UiLanguage::Text(L"请先选择一个项目。")); return; }
    OnSelectionChanged();
    BOOL valid; UINT count = GetDlgItemInt(IDC_EDITNUMBER, &valid, FALSE);
    m_nSelectedCount = valid ? max(1, min((int)min(count, (UINT)COUNT_MAX), CurrentLimit())) : 1;
    if (m_EditPossibility.IsWindowEnabled()) {
        CString text; m_EditPossibility.GetWindowText(text); text.Trim(); wchar_t* end = nullptr;
        double chance = wcstod(text.GetString(), &end);
        if (end == text.GetString() || *end || !std::isfinite(chance) ||
            (m_weights ? chance <= 0 || chance > 1000000 : chance < 0 || chance > 100)) {
            AfxMessageBox(m_weights ? UiLanguage::Text(L"权重须大于 0 且不超过 1000000。") : UiLanguage::Text(L"概率须为 0–100%，支持小数。")); return;
        }
        m_chance = chance;
    } else m_chance = 100;
    m_strSelectedItem = id; CDialogEx::OnOK();
}
void CDlgSelect::OnChanceSpin(NMHDR* header, LRESULT* result) {
    auto delta = reinterpret_cast<NMUPDOWN*>(header); CString text; m_EditPossibility.GetWindowText(text);
    double value = wcstod(text, nullptr) - delta->iDelta;
    value = max(m_weights ? 0.01 : 0.0, min(m_weights ? 1000000.0 : 100.0, value));
    text.Format(_T("%.12g"), value); m_EditPossibility.SetWindowText(text); *result = 1;
}
BOOL CDlgSelect::PreTranslateMessage(MSG* msg) {
    if (msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN && msg->hwnd == m_EditSearch.GetSafeHwnd()) {
        m_List.SetFocus(); return TRUE;
    }
    return CDialogEx::PreTranslateMessage(msg);
}
void CDlgSelect::OnEnChangeEditSearch() {
    CString current; GetCurSelItem(current); RefreshList(current); OnSelectionChanged();
}
void CDlgSelect::OnLbnDblclkList() { OnOK(); }
void CDlgSelect::OnItemTagChanged() {
    CString current; GetCurSelItem(current); m_preferred[m_tagMode ? 1 : 0] = current;
    const int selected = m_ItemTag.GetCurSel(); if (selected == CB_ERR) return;
    m_tagMode = m_allowTags && m_ItemTag.GetItemData(selected) == 1;
    m_EditSearch.SetWindowText(L"");
    m_EditSearch.SendMessage(EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(UiLanguage::Text(m_tagMode ? L"搜索或填写标签 ID" : L"搜索物品 ID")));
    RefreshList(m_preferred[m_tagMode ? 1 : 0]); OnSelectionChanged();
}
BEGIN_MESSAGE_MAP(CDlgSelect, CDialogEx)
    ON_CBN_SELCHANGE(IDC_COMBOITEMTAG, &CDlgSelect::OnItemTagChanged)
    ON_EN_CHANGE(IDC_EDITSEARCH, &CDlgSelect::OnEnChangeEditSearch)
    ON_LBN_DBLCLK(IDC_LIST, &CDlgSelect::OnLbnDblclkList)
    ON_LBN_SELCHANGE(IDC_LIST, &CDlgSelect::OnSelectionChanged)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPINPOSSIBILITY, &CDlgSelect::OnChanceSpin)
END_MESSAGE_MAP()
