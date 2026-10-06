#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgSelect.h"
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
}
void CDlgSelect::SetItemList(const CStringArray* list) { m_pItemList = list; }
void CDlgSelect::SetInitialSelection(const CString& id, int count) { m_strInitialItem = id; m_nInitialCount = count; }
void CDlgSelect::SetCountLimit(int count) { m_nMaxCount = max(1, min(COUNT_MAX, count)); }
int CDlgSelect::CurrentLimit() const { return m_kind == RecipeFluid ? COUNT_MAX : m_nMaxCount; }
BOOL CDlgSelect::OnInitDialog() {
    CDialogEx::OnInitDialog();
    m_SpinNumber.ModifyStyle(0, UDS_NOTHOUSANDS);
    m_SpinNumber.SetRange32(1, CurrentLimit()); m_SpinNumber.SetPos32(max(1, min(CurrentLimit(), m_nInitialCount)));
    // Decimal chance/weight editing must not be overwritten by an integer buddy.
    m_SpinPossibility.ModifyStyle(UDS_SETBUDDYINT, 0); m_SpinPossibility.SetRange32(0, 1000000);
    CString value; value.Format(_T("%.12g"), m_chance); m_EditPossibility.SetWindowText(value);
    GetDlgItem(IDC_GROUPCHANCE)->SetWindowText(m_weights ? _T("权重") : _T("概率%"));
    RefreshList(m_strInitialItem); OnSelectionChanged(); return TRUE;
}
void CDlgSelect::RefreshList(const CString& prefer) {
    m_List.ResetContent(); m_choices.clear();
    CString search; m_EditSearch.GetWindowText(search); search.MakeLower();
    CStringArray keywords; int pos = 0;
    CString word = search.Tokenize(_T(" \t"), pos);
    while (!word.IsEmpty()) { keywords.Add(word); word = search.Tokenize(_T(" \t"), pos); }
    int selected = LB_ERR;
    if (m_pItemList) for (INT_PTR i = 0; i < m_pItemList->GetSize(); ++i) {
        CString id = m_pItemList->GetAt(i), lower(id); lower.MakeLower(); bool match = true;
        for (INT_PTR k = 0; k < keywords.GetSize(); ++k) if (lower.Find(keywords[k]) < 0) match = false;
        if (!match) continue;
        unsigned kinds = RecipeKinds(m_catalog, std::wstring(id.GetString())) & m_allowedKinds;
        for (auto kind : { RecipeItem, RecipeFluid }) {
            if (!(kinds & kind)) continue;
            CString display = id;
            if (kind == RecipeFluid) display += _T("  [流体]");
            if (m_List.FindStringExact(-1, display) != LB_ERR) continue;
            m_choices.push_back({ id, kind });
            int index = m_List.AddString(display);
            if (index >= 0) m_List.SetItemData(index, m_choices.size());
        }
    }
    if (m_choices.empty()) {
        int index = m_List.AddString(_T("（没有匹配的项目，请导入相应 jar 或修改搜索词）"));
        if (index >= 0) m_List.SetItemData(index, 0);
    } else {
        for (int i = 0; i < m_List.GetCount(); ++i) {
            auto data = m_List.GetItemData(i);
            if (data && data <= m_choices.size() && m_choices[data - 1].id == prefer && m_choices[data - 1].kind == m_kind) selected = i;
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
    GetDlgItem(IDC_GROUPCOUNT)->SetWindowText(m_kind == RecipeFluid ? _T("体积(mB)") : _T("数量"));
    auto app = static_cast<CKubeJSRecipeHelperApp*>(AfxGetApp());
    BOOL chance = (m_hasChanceMode ? m_allowChance : app->m_bIsCreate) && m_kind == RecipeItem;
    m_EditPossibility.EnableWindow(chance); m_SpinPossibility.EnableWindow(chance);
}
void CDlgSelect::OnOK() {
    CString id; if (!GetCurSelItem(id)) { AfxMessageBox(_T("请先选择一个项目。")); return; }
    OnSelectionChanged();
    BOOL valid; UINT count = GetDlgItemInt(IDC_EDITNUMBER, &valid, FALSE);
    m_nSelectedCount = valid ? max(1, min((int)min(count, (UINT)COUNT_MAX), CurrentLimit())) : 1;
    if (m_EditPossibility.IsWindowEnabled()) {
        CString text; m_EditPossibility.GetWindowText(text); text.Trim(); wchar_t* end = nullptr;
        double chance = wcstod(text.GetString(), &end);
        if (end == text.GetString() || *end || !std::isfinite(chance) ||
            (m_weights ? chance <= 0 || chance > 1000000 : chance < 0 || chance > 100)) {
            AfxMessageBox(m_weights ? _T("权重须大于 0 且不超过 1000000。") : _T("概率须为 0–100%，支持小数。")); return;
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
BEGIN_MESSAGE_MAP(CDlgSelect, CDialogEx)
    ON_EN_CHANGE(IDC_EDITSEARCH, &CDlgSelect::OnEnChangeEditSearch)
    ON_LBN_DBLCLK(IDC_LIST, &CDlgSelect::OnLbnDblclkList)
    ON_LBN_SELCHANGE(IDC_LIST, &CDlgSelect::OnSelectionChanged)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPINPOSSIBILITY, &CDlgSelect::OnChanceSpin)
END_MESSAGE_MAP()
