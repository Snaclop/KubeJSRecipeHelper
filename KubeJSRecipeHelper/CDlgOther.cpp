#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgOther.h"
#include "CDlgSelect.h"
#include "Language.h"
#include "NumberText.h"
#include <cmath>
#include <cwchar>

using namespace ModRecipe;
namespace {
enum Controls { RecipeType = 2300, TargetVersion, FieldPage, GroupBase = 2400, FieldBase = 2600 };
CString Number(double dValue) { return FormatRecipeNumber(dValue).c_str(); }
}
IMPLEMENT_DYNAMIC(CDlgOther, CDialogEx)
CDlgOther::CDlgOther(CWnd* pParent) : CDialogEx(IDD_DLGOTHER, pParent) {}
CDlgOther::~CDlgOther() {}
void CDlgOther::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); DDX_Control(pDX, IDC_COMBOOTHER, m_comboMods); }
const Recipe& CDlgOther::Recipe() const { return m_definitions.arrMods[m_nModIndex].arrRecipes[m_arrRecipeIndices[m_nModIndex]]; }
Draft& CDlgOther::Draft() { return m_arrDrafts[m_nModIndex][m_arrRecipeIndices[m_nModIndex]]; }
Entry& CDlgOther::EntryAt(const Slot& slot) { return Draft().mapGroups[Recipe().arrGroups[slot.nGroupIndex].strId][slot.nIndex]; }
CString CDlgOther::Label(const Json& jsonLabel) const { return ModRecipe::Label(jsonLabel, m_strLocale).c_str(); }
CWnd* CDlgOther::Control(LPCTSTR pszClass, const CString& strText, UINT uId, int nX, int nY, int nWidth, int nHeight, DWORD dwStyle) {
    auto pControl = std::make_unique<CWnd>();
    if (!pControl->Create(pszClass, strText, WS_CHILD | WS_VISIBLE | dwStyle, m_ui.Rect(nX, nY, nWidth, nHeight), this, uId)) return nullptr;
    pControl->SetFont(GetFont()); auto pResult = pControl.get(); m_arrControls.push_back(std::move(pControl)); return pResult;
}
CWnd* CDlgOther::Combo(UINT uId, int nX, int nY, int nWidth, const std::vector<CString>& arrChoices, int nSelectedIndex) {
    auto pControl = Control(L"COMBOBOX", L"", uId, nX, nY, nWidth, 220, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    if (pControl) {
        for (const auto& strText : arrChoices) pControl->SendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strText.GetString()));
        pControl->SendMessage(CB_SETCURSEL, nSelectedIndex);
    }
    return pControl;
}
BOOL CDlgOther::OnInitDialog() {
    CDialogEx::OnInitDialog(); UiLanguage::ApplyDialog(*this); m_bRebuilding = true;
    m_strLocale = UiLanguage::Get() == UiLanguage::Language::English ? "en" :
        UiLanguage::Get() == UiLanguage::Language::TraditionalChinese ? "zh-TW" : "zh-CN";
    try { m_definitions = LoadDirectory(Directory()); }
    catch (const std::exception& entry) { m_definitions.arrErrors.push_back(Wide(entry.what())); }
    if (m_definitions.arrMods.empty() || !m_definitions.arrErrors.empty()) {
        CString strMessage = m_definitions.arrMods.empty()
            ? UiLanguage::Text(L"没有可用的模组配置。请将模组 JSON 放入程序目录下的 addon 文件夹。")
            : UiLanguage::Text(L"以下模组配置无法读取，其他有效配置仍可使用：\r\n");
        for (const auto& strError : m_definitions.arrErrors) { strMessage += L"\r\n\r\n"; strMessage += strError.c_str(); }
        AfxMessageBox(strMessage, m_definitions.arrMods.empty() ? MB_ICONINFORMATION : MB_ICONWARNING);
    }
    if (m_definitions.arrMods.empty()) { EndDialog(IDCANCEL); return FALSE; }
    m_ui.Initialize(this, 1000, 760);
    for (size_t nIndex = 0; nIndex < m_definitions.arrMods.size(); ++nIndex) {
        const auto& mod = m_definitions.arrMods[nIndex];
        int nItemIndex = m_comboMods.AddString(Wide(mod.strName).c_str()); m_comboMods.SetItemData(nItemIndex, nIndex);
        m_arrRecipeIndices.push_back(0); m_arrDrafts.emplace_back();
        for (const auto& recipe : mod.arrRecipes) m_arrDrafts.back().push_back(MakeDraft(recipe));
    }
    // Resource sorting may change display order; item data identifies the definition.
    m_nModIndex = (int)m_comboMods.GetItemData(0); m_comboMods.SetCurSel(0); m_ui.Move(IDC_COMBOOTHER, 82, 16, 280, 250);
    m_comboMethods.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, m_ui.Rect(470, 16, 285, 250), this, RecipeType);
    m_comboVersions.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, m_ui.Rect(862, 16, 115, 170), this, TargetVersion);
    m_comboMethods.SetFont(GetFont()); m_comboVersions.SetFont(GetFont()); PopulateMethods(); PopulateVersions();
    Rebuild(); return TRUE;
}
void CDlgOther::PopulateMethods() {
    m_comboMethods.ResetContent();
    for (const auto& recipe : m_definitions.arrMods[m_nModIndex].arrRecipes) m_comboMethods.AddString(Label(recipe.jsonLabel));
    m_comboMethods.SetCurSel(m_arrRecipeIndices[m_nModIndex]);
}
void CDlgOther::PopulateVersions() {
    m_comboVersions.ResetContent(); const auto& arrVersions = Recipe().arrVersions;
    int nSelectedIndex = (int)arrVersions.size() - 1;
    for (size_t nIndex = 0; nIndex < arrVersions.size(); ++nIndex) {
        m_comboVersions.AddString(Wide(arrVersions[nIndex]).c_str()); if (arrVersions[nIndex] == m_strVersion) nSelectedIndex = (int)nIndex;
    }
    m_comboVersions.SetCurSel(nSelectedIndex); m_strVersion = arrVersions[nSelectedIndex];
}
void CDlgOther::SaveControls() {
    auto& jsonFields = Draft().jsonFields;
    for (size_t nIndex : m_arrVisibleFields) {
        const auto& field = Recipe().arrFields[nIndex]; CWnd* pControl = GetDlgItem(FieldBase + (UINT)nIndex); if (!pControl) continue;
        CString strText; pControl->GetWindowText(strText);
        if (field.strType == "boolean") jsonFields[field.strId] = pControl->SendMessage(BM_GETCHECK) == BST_CHECKED;
        else if (field.strType == "choice") {
            const int nSelectedIndex = (int)pControl->SendMessage(CB_GETCURSEL);
            if (nSelectedIndex >= 0 && nSelectedIndex < (int)field.jsonChoices.size()) jsonFields[field.strId] = field.jsonChoices[nSelectedIndex]["value"];
            else if (field.bAllowCustom) { strText.Trim(); jsonFields[field.strId] = Utf8(strText.GetString()); }
            else jsonFields[field.strId] = nullptr;
        } else if (field.strType == "integer" || field.strType == "number") {
            strText.Trim(); wchar_t* pEnd = nullptr; double dValue = wcstod(strText.GetString(), &pEnd);
            if (strText.IsEmpty() || pEnd == strText.GetString() || *pEnd || !std::isfinite(dValue)) jsonFields[field.strId] = nullptr;
            else if (field.strType == "integer" && std::floor(dValue) == dValue && dValue >= -9007199254740991.0 && dValue <= 9007199254740991.0)
                jsonFields[field.strId] = static_cast<int64_t>(dValue);
            else jsonFields[field.strId] = dValue;
        } else {
            if (field.strType != "text") strText.Trim(); jsonFields[field.strId] = Utf8(strText.GetString());
        }
    }
}
void CDlgOther::BuildGroups(bool bOutput) {
    const auto& recipe = Recipe(); auto& draft = Draft(); std::vector<size_t> arrGroups;
    for (size_t nIndex = 0; nIndex < recipe.arrGroups.size(); ++nIndex) {
        const auto& group = recipe.arrGroups[nIndex];
        if ((group.strSide == "output") == bOutput && Visible(group.jsonWhen, draft)) arrGroups.push_back(nIndex);
    }
    if (arrGroups.empty()) return;
    const int nWidth = 448 / (int)arrGroups.size();
    for (size_t nColumn = 0; nColumn < arrGroups.size(); ++nColumn) {
        const size_t nGroupIndex = arrGroups[nColumn]; const auto& group = recipe.arrGroups[nGroupIndex]; auto& arrEntries = draft.mapGroups[group.strId];
        const int nX = (bOutput ? 530 : 22) + (int)nColumn * nWidth;
        const int nColumns = (std::max)(1, (std::min)(3, nWidth / 79)); const int nPerPage = nColumns * 3;
        int& nPage = m_mapGroupPages[group.strId]; const int nPages = ((int)arrEntries.size() + nPerPage - 1) / nPerPage;
        nPage = (std::max)(0, (std::min)(nPage, nPages - 1));
        CString strTitle; strTitle.Format(L"%s (%d–%d)", Label(group.jsonLabel).GetString(), group.nMinimum, group.nMaxTotal);
        Control(L"STATIC", strTitle, 0, nX, 66, nWidth - 6, 25);
        if (group.nMaximum > 1) {
            Control(L"BUTTON", L"+", GroupBase + (UINT)nGroupIndex * 3, nX, 96, 28, 26, BS_PUSHBUTTON | WS_TABSTOP);
            Control(L"BUTTON", L"−", GroupBase + (UINT)nGroupIndex * 3 + 1, nX + 32, 96, 28, 26, BS_PUSHBUTTON | WS_TABSTOP);
        }
        if (nPages > 1) {
            std::vector<CString> arrLabels;
            for (int nIndex = 0; nIndex < nPages; ++nIndex) { CString strText; strText.Format(L"%d/%d", nIndex + 1, nPages); arrLabels.push_back(strText); }
            Combo(GroupBase + (UINT)nGroupIndex * 3 + 2, nX + 64, 96, nWidth - 70, arrLabels, nPage);
        }
        for (int nIndex = nPage * nPerPage; nIndex < (std::min)((int)arrEntries.size(), (nPage + 1) * nPerPage); ++nIndex) {
            const int nLocalIndex = nIndex - nPage * nPerPage; CString strLabel; strLabel.Format(L"%s %d", Label(group.jsonLabel).GetString(), nIndex + 1);
            m_arrSlots.push_back({nGroupIndex, (size_t)nIndex, m_ui.Rect(nX + (nLocalIndex % nColumns) * 79, 136 + (nLocalIndex / nColumns) * 82, 58, 58), strLabel});
        }
    }
}
void CDlgOther::BuildFields() {
    const auto& recipe = Recipe(); auto& draft = Draft(); std::vector<size_t> arrVisible;
    for (size_t nIndex = 0; nIndex < recipe.arrFields.size(); ++nIndex) if (Visible(recipe.arrFields[nIndex].jsonWhen, draft)) arrVisible.push_back(nIndex);
    const int nPages = (std::max)(1, ((int)arrVisible.size() + 9) / 10);
    m_nFieldPage = (std::max)(0, (std::min)(m_nFieldPage, nPages - 1));
    if (nPages > 1) {
        std::vector<CString> arrLabels; for (int nIndex = 0; nIndex < nPages; ++nIndex) { CString strPage; strPage.Format(L"%d/%d", nIndex + 1, nPages); arrLabels.push_back(strPage); }
        Combo(FieldPage, 862, 380, 115, arrLabels, m_nFieldPage);
    }
    for (int nVisibleIndex = m_nFieldPage * 10; nVisibleIndex < (std::min)((int)arrVisible.size(), (m_nFieldPage + 1) * 10); ++nVisibleIndex) {
        const size_t nIndex = arrVisible[nVisibleIndex]; const auto& field = recipe.arrFields[nIndex]; const auto& jsonValue = draft.jsonFields[field.strId];
        const int nLocalIndex = nVisibleIndex % 10, nX = nLocalIndex % 2 ? 530 : 22, nY = 420 + (nLocalIndex / 2) * 43;
        const UINT uId = FieldBase + (UINT)nIndex; m_arrVisibleFields.push_back(nIndex);
        if (field.strType == "boolean") {
            auto pControl = Control(L"BUTTON", Label(field.jsonLabel), uId, nX, nY, 440, 29, BS_AUTOCHECKBOX | WS_TABSTOP);
            if (pControl) pControl->SendMessage(BM_SETCHECK, jsonValue == true ? BST_CHECKED : BST_UNCHECKED);
            continue;
        }
        Control(L"STATIC", Label(field.jsonLabel), 0, nX, nY + 3, 155, 28);
        if (field.strType == "choice") {
            auto pControl = Control(L"COMBOBOX", L"", uId, nX + 160, nY, 282, 230,
                WS_TABSTOP | WS_VSCROLL | (field.bAllowCustom ? CBS_DROPDOWN | CBS_AUTOHSCROLL : CBS_DROPDOWNLIST));
            if (!pControl) continue;
            int nSelectedIndex = CB_ERR;
            for (size_t nChoiceIndex = 0; nChoiceIndex < field.jsonChoices.size(); ++nChoiceIndex) {
                CString strText = Label(field.jsonChoices[nChoiceIndex]["label"]);
                pControl->SendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strText.GetString()));
                if (field.jsonChoices[nChoiceIndex]["value"] == jsonValue) nSelectedIndex = (int)nChoiceIndex;
            }
            pControl->SendMessage(CB_SETCURSEL, nSelectedIndex);
            if (nSelectedIndex == CB_ERR && jsonValue.is_string()) pControl->SetWindowText(Wide(jsonValue.get<std::string>()).c_str());
        } else {
            CString strText;
            if (jsonValue.is_number_integer()) strText = Wide(jsonValue.dump()).c_str();
            else if (jsonValue.is_number()) strText = Number(jsonValue.get<double>());
            else if (jsonValue.is_string()) strText = Wide(jsonValue.get<std::string>()).c_str();
            auto pControl = Control(L"EDIT", strText, uId, nX + 160, nY, 282, 28, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
            if (pControl) pControl->SendMessage(EM_SETLIMITTEXT, 4096);

        }
    }
}
void CDlgOther::Rebuild() {
    m_bRebuilding = true; m_ui.ClearTips();
    for (auto& pControl : m_arrControls) if (pControl->GetSafeHwnd()) pControl->DestroyWindow();
    m_arrControls.clear(); m_arrSlots.clear(); m_arrVisibleFields.clear(); m_nHoverIndex = -1;
    Control(L"STATIC", UiLanguage::Text(L"模组"), 0, 22, 20, 55, 25);
    Control(L"STATIC", UiLanguage::Text(L"配方类型"), 0, 380, 20, 85, 25);
    Control(L"STATIC", UiLanguage::Text(L"目标版本"), 0, 765, 20, 95, 25);
    BuildGroups(false); BuildGroups(true); BuildFields();
    Control(L"STATIC", UiLanguage::Text(L"左键选择，右键清空；悬停查看 ID、数量和概率。分组可留空。"), 0, 22, 660, 950, 28);
    m_comboMods.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    m_comboMethods.SetWindowPos(&m_comboMods, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    m_comboVersions.SetWindowPos(&m_comboMethods, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    CWnd* pPrevious = &m_comboVersions;
    for (auto& pControl : m_arrControls) { pControl->SetWindowPos(pPrevious, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); pPrevious = pControl.get(); }
    GetDlgItem(IDOK)->SetWindowPos(pPrevious, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    GetDlgItem(IDCANCEL)->SetWindowPos(GetDlgItem(IDOK), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    UpdateTips(); m_bRebuilding = false; Invalidate(TRUE);
}
void CDlgOther::UpdateTips() {
    for (size_t nIndex = 0; nIndex < m_arrSlots.size(); ++nIndex) {
        const auto& slot = m_arrSlots[nIndex]; const auto& entry = EntryAt(slot); const auto& group = Recipe().arrGroups[slot.nGroupIndex]; CString strLabel = slot.strLabel;
        if (group.bFluid) strLabel += L" (mB)";
        if (group.bAllowChance && !entry.strId.empty()) { CString strChance; strChance.Format(UiLanguage::Text(L"（概率 %.12g%%）"), entry.dChance); strLabel += strChance; }
        m_ui.Tip(nIndex + 1, slot.rcRect, strLabel, entry.strId.c_str(), entry.nCount);
    }
}
BOOL CDlgOther::OnCommand(WPARAM wParam, LPARAM lParam) {
    if (m_bRebuilding || m_definitions.arrMods.empty()) return CDialogEx::OnCommand(wParam, lParam);
    const UINT uId = LOWORD(wParam), uNotification = HIWORD(wParam);
    if (uNotification == CBN_SELCHANGE && (uId == IDC_COMBOOTHER || uId == RecipeType || uId == TargetVersion)) {
        SaveControls();
        if (uId == IDC_COMBOOTHER && m_comboMods.GetCurSel() >= 0) { m_nModIndex = (int)m_comboMods.GetItemData(m_comboMods.GetCurSel()); PopulateMethods(); PopulateVersions(); }
        if (uId == RecipeType && m_comboMethods.GetCurSel() >= 0) { m_arrRecipeIndices[m_nModIndex] = m_comboMethods.GetCurSel(); PopulateVersions(); }
        if (uId == TargetVersion && m_comboVersions.GetCurSel() >= 0) m_strVersion = Recipe().arrVersions[m_comboVersions.GetCurSel()];
        m_mapGroupPages.clear(); m_nFieldPage = 0; Rebuild(); return TRUE;
    }
    if (uId == FieldPage && uNotification == CBN_SELCHANGE) {
        SaveControls(); m_nFieldPage = (int)GetDlgItem(uId)->SendMessage(CB_GETCURSEL); Rebuild(); return TRUE;
    }
    if (uId >= GroupBase && uId < GroupBase + Recipe().arrGroups.size() * 3) {
        const size_t nIndex = (uId - GroupBase) / 3; const int nAction = (uId - GroupBase) % 3;
        const auto& group = Recipe().arrGroups[nIndex]; auto& arrEntries = Draft().mapGroups[group.strId];
        if ((nAction == 2 && uNotification == CBN_SELCHANGE) || (nAction != 2 && uNotification == BN_CLICKED)) {
            SaveControls();
            if (nAction == 2) m_mapGroupPages[group.strId] = (int)GetDlgItem(uId)->SendMessage(CB_GETCURSEL);
            else if (nAction == 0) {
                int64_t nUsed = 0; for (const auto& entry : arrEntries) nUsed += group.bTotalByCount ? (std::max)(1, entry.nCount) : 1;
                if (arrEntries.size() >= (size_t)group.nMaximum || nUsed >= group.nMaxTotal) { AfxMessageBox(UiLanguage::Text(L"此类型格子或物品总数量已达到该配方的上限。")); return TRUE; }
                arrEntries.emplace_back();
                int nVisibleGroups = 0; for (const auto& otherGroup : Recipe().arrGroups) if (otherGroup.strSide == group.strSide && Visible(otherGroup.jsonWhen, Draft())) ++nVisibleGroups;
                const int nColumns = (std::max)(1, (std::min)(3, (448 / nVisibleGroups) / 79));
                m_mapGroupPages[group.strId] = ((int)arrEntries.size() - 1) / (nColumns * 3);
            } else if (arrEntries.size() > (size_t)(std::max)(1, group.nMinimum)) arrEntries.pop_back();
            Rebuild(); return TRUE;
        }
    }
    if (uId >= FieldBase && uId < FieldBase + Recipe().arrFields.size() &&
        (uNotification == CBN_SELCHANGE || (uNotification == BN_CLICKED && Recipe().arrFields[uId - FieldBase].strType == "boolean"))) {
        SaveControls(); Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wParam, lParam);
}
int CDlgOther::Hit(CPoint ptPoint) const { for (size_t nIndex = 0; nIndex < m_arrSlots.size(); ++nIndex) if (m_arrSlots[nIndex].rcRect.PtInRect(ptPoint)) return (int)nIndex; return -1; }
void CDlgOther::Select(const Slot& slot) {
    const auto& group = Recipe().arrGroups[slot.nGroupIndex]; const auto eKind = group.bFluid ? RecipeFluid : RecipeItem;
    bool bAvailable = false;
    if (m_pItemSource) for (INT_PTR nIndex = 0; nIndex < m_pItemSource->GetSize(); ++nIndex)
        if (RecipeKinds(m_pCatalog, std::wstring(m_pItemSource->GetAt(nIndex).GetString())) & eKind) { bAvailable = true; break; }
    if (!bAvailable && !group.bAllowTags) { AfxMessageBox(UiLanguage::Text(L"请先在主界面导入包含所需物品或流体的 jar 文件。"), MB_ICONINFORMATION); return; }
    SaveControls(); auto& entry = EntryAt(slot); int nLimit = group.nMaxCount;
    if (group.bTotalByCount) {
        int64_t nUsed = 0; for (const auto& other : Draft().mapGroups[group.strId]) if (&other != &entry && !other.strId.empty()) nUsed += other.nCount;
        const int64_t nRemaining = (std::max)(int64_t(0), (int64_t)group.nMaxTotal - nUsed);
        nLimit = (int)(std::min)((int64_t)nLimit, nRemaining);
    }
    if (nLimit < 1) { AfxMessageBox(UiLanguage::Text(L"此类型格子或物品总数量已达到该配方的上限。")); return; }
    CDlgSelect select(this); select.SetItemList(m_pItemSource); select.SetCatalog(m_pCatalog, eKind);
    select.SetAllowTags(group.bAllowTags);
    select.SetCountLimit(nLimit, group.bFluid); select.SetChanceMode(group.bAllowChance);
    select.SetInitialSelection(entry.strId.c_str(), entry.nCount); select.SetInitialDetails(eKind, entry.dChance);
    if (select.DoModal() == IDOK) {
        entry.strId = select.GetSelectedItem().GetString(); entry.nCount = select.GetSelectedCount(); entry.dChance = select.GetSelectedChance();
        UpdateTips(); InvalidateRect(slot.rcRect, FALSE);
    }
}
void CDlgOther::OnPaint() {
    CPaintDC dc(this); if (m_definitions.arrMods.empty()) return;
    m_ui.Arrow(&dc, m_ui.Rect(478, 194, 42, 58));
    for (size_t nIndex = 0; nIndex < m_arrSlots.size(); ++nIndex) {
        const auto& slot = m_arrSlots[nIndex]; auto& entry = EntryAt(slot);
        m_ui.Slot(&dc, slot.rcRect, m_nHoverIndex == (int)nIndex, entry.strId.c_str(), entry.nCount, m_pTextures);
        CRect rcLabel = slot.rcRect; int nHeight = rcLabel.Height(); rcLabel.top = rcLabel.bottom + 3; rcLabel.bottom = rcLabel.top + nHeight / 3;
        m_ui.Text(&dc, slot.strLabel, rcLabel, false, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        if (Recipe().arrGroups[slot.nGroupIndex].bAllowChance && !entry.strId.empty() && entry.dChance != 100) {
            CString strChance; strChance.Format(L"%.4g%%", entry.dChance); CRect rcRect = slot.rcRect; rcRect.bottom = rcRect.top + nHeight / 3;
            int nSavedDC = dc.SaveDC(); dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
            CRect rcShadow = rcRect; rcShadow.OffsetRect(1, 1); dc.SetTextColor(RGB(55, 55, 55)); dc.DrawText(strChance, rcShadow, DT_LEFT | DT_SINGLELINE);
            dc.SetTextColor(RGB(255, 255, 255)); dc.DrawText(strChance, rcRect, DT_LEFT | DT_SINGLELINE); dc.RestoreDC(nSavedDC);
        }
    }
}
void CDlgOther::OnLButtonDown(UINT uFlags, CPoint ptPoint) { int nItemIndex = Hit(ptPoint); if (nItemIndex >= 0) Select(m_arrSlots[nItemIndex]); CDialogEx::OnLButtonDown(uFlags, ptPoint); }
void CDlgOther::OnRButtonDown(UINT uFlags, CPoint ptPoint) { int nItemIndex = Hit(ptPoint); if (nItemIndex >= 0) { EntryAt(m_arrSlots[nItemIndex]) = Entry(); UpdateTips(); InvalidateRect(m_arrSlots[nItemIndex].rcRect, FALSE); } CDialogEx::OnRButtonDown(uFlags, ptPoint); }
void CDlgOther::OnMouseMove(UINT uFlags, CPoint ptPoint) {
    int nItemIndex = Hit(ptPoint); if (nItemIndex != m_nHoverIndex) { int nPreviousHover = m_nHoverIndex; m_nHoverIndex = nItemIndex; if (nPreviousHover >= 0) InvalidateRect(m_arrSlots[nPreviousHover].rcRect, FALSE); if (nItemIndex >= 0) InvalidateRect(m_arrSlots[nItemIndex].rcRect, FALSE); }
    if (!m_bTracking) { TRACKMOUSEEVENT event = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0}; m_bTracking = TrackMouseEvent(&event) != FALSE; }
    CDialogEx::OnMouseMove(uFlags, ptPoint);
}
void CDlgOther::OnMouseLeave() { m_bTracking = false; if (m_nHoverIndex >= 0) InvalidateRect(m_arrSlots[m_nHoverIndex].rcRect, FALSE); m_nHoverIndex = -1; CDialogEx::OnMouseLeave(); }
BOOL CDlgOther::PreTranslateMessage(MSG* pMessage) { m_ui.Relay(pMessage); return CDialogEx::PreTranslateMessage(pMessage); }
void CDlgOther::OnOK() {
    if (m_definitions.arrMods.empty()) return;
    SaveControls(); Json jsonBody; std::wstring strError;
    if (!Build(Recipe(), m_strVersion, Draft(), jsonBody, strError, m_strLocale)) { AfxMessageBox(strError.c_str(), MB_ICONINFORMATION); return; }
    m_strScript = Script(jsonBody).c_str(); CDialogEx::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgOther, CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
