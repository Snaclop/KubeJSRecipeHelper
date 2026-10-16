#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgOther.h"
#include "CDlgSelect.h"
#include "Language.h"
#include <cmath>
#include <cwchar>

using namespace ModRecipe;
namespace {
enum Controls { RecipeType = 2300, TargetVersion, FieldPage, GroupBase = 2400, FieldBase = 2600 };
CString Number(double value) { CString text; text.Format(L"%.12g", value); return text; }
}
IMPLEMENT_DYNAMIC(CDlgOther, CDialogEx)
CDlgOther::CDlgOther(CWnd* parent) : CDialogEx(IDD_DLGOTHER, parent) {}
CDlgOther::~CDlgOther() {}
void CDlgOther::DoDataExchange(CDataExchange* dx) { CDialogEx::DoDataExchange(dx); DDX_Control(dx, IDC_COMBOOTHER, m_mods); }
const Recipe& CDlgOther::Recipe() const { return m_definitions.mods[m_mod].recipes[m_recipes[m_mod]]; }
Draft& CDlgOther::Draft() { return m_drafts[m_mod][m_recipes[m_mod]]; }
Entry& CDlgOther::EntryAt(const Slot& slot) { return Draft().groups[Recipe().groups[slot.group].id][slot.index]; }
CString CDlgOther::Label(const Json& text) const { return ModRecipe::Label(text, m_locale).c_str(); }
CWnd* CDlgOther::Control(LPCTSTR cls, const CString& text, UINT id, int x, int y, int w, int h, DWORD style) {
    auto control = std::make_unique<CWnd>();
    if (!control->Create(cls, text, WS_CHILD | WS_VISIBLE | style, m_ui.Rect(x, y, w, h), this, id)) return nullptr;
    control->SetFont(GetFont()); auto result = control.get(); m_controls.push_back(std::move(control)); return result;
}
CWnd* CDlgOther::Combo(UINT id, int x, int y, int w, const std::vector<CString>& choices, int selected) {
    auto control = Control(L"COMBOBOX", L"", id, x, y, w, 220, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    if (control) {
        for (const auto& text : choices) control->SendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.GetString()));
        control->SendMessage(CB_SETCURSEL, selected);
    }
    return control;
}
BOOL CDlgOther::OnInitDialog() {
    CDialogEx::OnInitDialog(); UiLanguage::ApplyDialog(*this); m_rebuilding = true;
    m_locale = UiLanguage::Get() == UiLanguage::Language::English ? "en" :
        UiLanguage::Get() == UiLanguage::Language::TraditionalChinese ? "zh-TW" : "zh-CN";
    try { m_definitions = LoadDirectory(Directory()); }
    catch (const std::exception& e) { m_definitions.errors.push_back(Wide(e.what())); }
    if (m_definitions.mods.empty() || !m_definitions.errors.empty()) {
        CString message = m_definitions.mods.empty()
            ? UiLanguage::Text(L"没有可用的模组配置。请将模组 JSON 放入程序目录下的 addon 文件夹。")
            : UiLanguage::Text(L"以下模组配置无法读取，其他有效配置仍可使用：\r\n");
        for (const auto& error : m_definitions.errors) { message += L"\r\n\r\n"; message += error.c_str(); }
        AfxMessageBox(message, m_definitions.mods.empty() ? MB_ICONINFORMATION : MB_ICONWARNING);
    }
    if (m_definitions.mods.empty()) { EndDialog(IDCANCEL); return FALSE; }
    m_ui.Initialize(this, 1000, 760);
    for (size_t i = 0; i < m_definitions.mods.size(); ++i) {
        const auto& mod = m_definitions.mods[i];
        int n = m_mods.AddString(Wide(mod.name).c_str()); m_mods.SetItemData(n, i);
        m_recipes.push_back(0); m_drafts.emplace_back();
        for (const auto& recipe : mod.recipes) m_drafts.back().push_back(MakeDraft(recipe));
    }
    // Resource sorting may change display order; item data identifies the definition.
    m_mod = (int)m_mods.GetItemData(0); m_mods.SetCurSel(0); m_ui.Move(IDC_COMBOOTHER, 82, 16, 280, 250);
    m_methods.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, m_ui.Rect(470, 16, 285, 250), this, RecipeType);
    m_versions.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, m_ui.Rect(862, 16, 115, 170), this, TargetVersion);
    m_methods.SetFont(GetFont()); m_versions.SetFont(GetFont()); PopulateMethods(); PopulateVersions();
    Rebuild(); return TRUE;
}
void CDlgOther::PopulateMethods() {
    m_methods.ResetContent();
    for (const auto& recipe : m_definitions.mods[m_mod].recipes) m_methods.AddString(Label(recipe.label));
    m_methods.SetCurSel(m_recipes[m_mod]);
}
void CDlgOther::PopulateVersions() {
    m_versions.ResetContent(); const auto& versions = Recipe().versions;
    int selected = (int)versions.size() - 1;
    for (size_t i = 0; i < versions.size(); ++i) {
        m_versions.AddString(Wide(versions[i]).c_str()); if (versions[i] == m_version) selected = (int)i;
    }
    m_versions.SetCurSel(selected); m_version = versions[selected];
}
void CDlgOther::SaveControls() {
    auto& fields = Draft().fields;
    for (size_t index : m_visibleFields) {
        const auto& field = Recipe().fields[index]; CWnd* control = GetDlgItem(FieldBase + (UINT)index); if (!control) continue;
        CString text; control->GetWindowText(text);
        if (field.type == "boolean") fields[field.id] = control->SendMessage(BM_GETCHECK) == BST_CHECKED;
        else if (field.type == "choice") {
            const int selected = (int)control->SendMessage(CB_GETCURSEL);
            if (selected >= 0 && selected < (int)field.choices.size()) fields[field.id] = field.choices[selected]["value"];
            else if (field.allowCustom) { text.Trim(); fields[field.id] = Utf8(text.GetString()); }
            else fields[field.id] = nullptr;
        } else if (field.type == "integer" || field.type == "number") {
            text.Trim(); wchar_t* end = nullptr; double value = wcstod(text.GetString(), &end);
            if (text.IsEmpty() || end == text.GetString() || *end || !std::isfinite(value)) fields[field.id] = nullptr;
            else if (field.type == "integer" && std::floor(value) == value && value >= -9007199254740991.0 && value <= 9007199254740991.0)
                fields[field.id] = static_cast<int64_t>(value);
            else fields[field.id] = value;
        } else {
            if (field.type != "text") text.Trim(); fields[field.id] = Utf8(text.GetString());
        }
    }
}
void CDlgOther::BuildGroups(bool output) {
    const auto& recipe = Recipe(); auto& draft = Draft(); std::vector<size_t> groups;
    for (size_t i = 0; i < recipe.groups.size(); ++i) {
        const auto& group = recipe.groups[i];
        if ((group.side == "output") == output && Visible(group.when, draft)) groups.push_back(i);
    }
    if (groups.empty()) return;
    const int width = 448 / (int)groups.size();
    for (size_t column = 0; column < groups.size(); ++column) {
        const size_t index = groups[column]; const auto& group = recipe.groups[index]; auto& entries = draft.groups[group.id];
        const int x = (output ? 530 : 22) + (int)column * width;
        const int columns = (std::max)(1, (std::min)(3, width / 79)); const int perPage = columns * 3;
        int& page = m_groupPages[group.id]; const int pages = ((int)entries.size() + perPage - 1) / perPage;
        page = (std::max)(0, (std::min)(page, pages - 1));
        CString title; title.Format(L"%s (%d–%d)", Label(group.label).GetString(), group.minimum, group.maxTotal);
        Control(L"STATIC", title, 0, x, 66, width - 6, 25);
        if (group.maximum > 1) {
            Control(L"BUTTON", L"+", GroupBase + (UINT)index * 3, x, 96, 28, 26, BS_PUSHBUTTON | WS_TABSTOP);
            Control(L"BUTTON", L"−", GroupBase + (UINT)index * 3 + 1, x + 32, 96, 28, 26, BS_PUSHBUTTON | WS_TABSTOP);
        }
        if (pages > 1) {
            std::vector<CString> labels;
            for (int i = 0; i < pages; ++i) { CString text; text.Format(L"%d/%d", i + 1, pages); labels.push_back(text); }
            Combo(GroupBase + (UINT)index * 3 + 2, x + 64, 96, width - 70, labels, page);
        }
        for (int i = page * perPage; i < (std::min)((int)entries.size(), (page + 1) * perPage); ++i) {
            const int local = i - page * perPage; CString label; label.Format(L"%s %d", Label(group.label).GetString(), i + 1);
            m_slots.push_back({index, (size_t)i, m_ui.Rect(x + (local % columns) * 79, 136 + (local / columns) * 82, 58, 58), label});
        }
    }
}
void CDlgOther::BuildFields() {
    const auto& recipe = Recipe(); auto& draft = Draft(); std::vector<size_t> visible;
    for (size_t i = 0; i < recipe.fields.size(); ++i) if (Visible(recipe.fields[i].when, draft)) visible.push_back(i);
    const int pages = (std::max)(1, ((int)visible.size() + 9) / 10);
    m_fieldPage = (std::max)(0, (std::min)(m_fieldPage, pages - 1));
    if (pages > 1) {
        std::vector<CString> labels; for (int i = 0; i < pages; ++i) { CString s; s.Format(L"%d/%d", i + 1, pages); labels.push_back(s); }
        Combo(FieldPage, 862, 380, 115, labels, m_fieldPage);
    }
    for (int i = m_fieldPage * 10; i < (std::min)((int)visible.size(), (m_fieldPage + 1) * 10); ++i) {
        const size_t index = visible[i]; const auto& field = recipe.fields[index]; const auto& value = draft.fields[field.id];
        const int local = i % 10, x = local % 2 ? 530 : 22, y = 420 + (local / 2) * 43;
        const UINT id = FieldBase + (UINT)index; m_visibleFields.push_back(index);
        if (field.type == "boolean") {
            auto control = Control(L"BUTTON", Label(field.label), id, x, y, 440, 29, BS_AUTOCHECKBOX | WS_TABSTOP);
            if (control) control->SendMessage(BM_SETCHECK, value == true ? BST_CHECKED : BST_UNCHECKED);
            continue;
        }
        Control(L"STATIC", Label(field.label), 0, x, y + 3, 155, 28);
        if (field.type == "choice") {
            auto control = Control(L"COMBOBOX", L"", id, x + 160, y, 282, 230,
                WS_TABSTOP | WS_VSCROLL | (field.allowCustom ? CBS_DROPDOWN | CBS_AUTOHSCROLL : CBS_DROPDOWNLIST));
            if (!control) continue;
            int selected = CB_ERR;
            for (size_t j = 0; j < field.choices.size(); ++j) {
                CString text = Label(field.choices[j]["label"]);
                control->SendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.GetString()));
                if (field.choices[j]["value"] == value) selected = (int)j;
            }
            control->SendMessage(CB_SETCURSEL, selected);
            if (selected == CB_ERR && value.is_string()) control->SetWindowText(Wide(value.get<std::string>()).c_str());
        } else {
            CString text;
            if (value.is_number()) text = Number(value.get<double>());
            else if (value.is_string()) text = Wide(value.get<std::string>()).c_str();
            auto control = Control(L"EDIT", text, id, x + 160, y, 282, 28, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
            if (control) control->SendMessage(EM_SETLIMITTEXT, 4096);

        }
    }
}
void CDlgOther::Rebuild() {
    m_rebuilding = true; m_ui.ClearTips();
    for (auto& control : m_controls) if (control->GetSafeHwnd()) control->DestroyWindow();
    m_controls.clear(); m_slots.clear(); m_visibleFields.clear(); m_hover = -1;
    Control(L"STATIC", UiLanguage::Text(L"模组"), 0, 22, 20, 55, 25);
    Control(L"STATIC", UiLanguage::Text(L"配方类型"), 0, 380, 20, 85, 25);
    Control(L"STATIC", UiLanguage::Text(L"目标版本"), 0, 765, 20, 95, 25);
    BuildGroups(false); BuildGroups(true); BuildFields();
    Control(L"STATIC", UiLanguage::Text(L"左键选择，右键清空；悬停查看 ID、数量和概率。分组可留空。"), 0, 22, 660, 950, 28);
    m_mods.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    m_methods.SetWindowPos(&m_mods, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    m_versions.SetWindowPos(&m_methods, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    CWnd* previous = &m_versions;
    for (auto& control : m_controls) { control->SetWindowPos(previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); previous = control.get(); }
    GetDlgItem(IDOK)->SetWindowPos(previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    GetDlgItem(IDCANCEL)->SetWindowPos(GetDlgItem(IDOK), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    UpdateTips(); m_rebuilding = false; Invalidate(TRUE);
}
void CDlgOther::UpdateTips() {
    for (size_t i = 0; i < m_slots.size(); ++i) {
        const auto& slot = m_slots[i]; const auto& e = EntryAt(slot); const auto& group = Recipe().groups[slot.group]; CString label = slot.label;
        if (group.fluid) label += L" (mB)";
        if (group.chance && !e.id.empty()) { CString chance; chance.Format(UiLanguage::Text(L"（概率 %.12g%%）"), e.chance); label += chance; }
        m_ui.Tip(i + 1, slot.rect, label, e.id.c_str(), e.count);
    }
}
BOOL CDlgOther::OnCommand(WPARAM wp, LPARAM lp) {
    if (m_rebuilding || m_definitions.mods.empty()) return CDialogEx::OnCommand(wp, lp);
    const UINT id = LOWORD(wp), notification = HIWORD(wp);
    if (notification == CBN_SELCHANGE && (id == IDC_COMBOOTHER || id == RecipeType || id == TargetVersion)) {
        SaveControls();
        if (id == IDC_COMBOOTHER && m_mods.GetCurSel() >= 0) { m_mod = (int)m_mods.GetItemData(m_mods.GetCurSel()); PopulateMethods(); PopulateVersions(); }
        if (id == RecipeType && m_methods.GetCurSel() >= 0) { m_recipes[m_mod] = m_methods.GetCurSel(); PopulateVersions(); }
        if (id == TargetVersion && m_versions.GetCurSel() >= 0) m_version = Recipe().versions[m_versions.GetCurSel()];
        m_groupPages.clear(); m_fieldPage = 0; Rebuild(); return TRUE;
    }
    if (id == FieldPage && notification == CBN_SELCHANGE) {
        SaveControls(); m_fieldPage = (int)GetDlgItem(id)->SendMessage(CB_GETCURSEL); Rebuild(); return TRUE;
    }
    if (id >= GroupBase && id < GroupBase + Recipe().groups.size() * 3) {
        const size_t index = (id - GroupBase) / 3; const int action = (id - GroupBase) % 3;
        const auto& group = Recipe().groups[index]; auto& entries = Draft().groups[group.id];
        if ((action == 2 && notification == CBN_SELCHANGE) || (action != 2 && notification == BN_CLICKED)) {
            SaveControls();
            if (action == 2) m_groupPages[group.id] = (int)GetDlgItem(id)->SendMessage(CB_GETCURSEL);
            else if (action == 0) {
                int64_t used = 0; for (const auto& e : entries) used += group.totalByCount ? (std::max)(1, e.count) : 1;
                if (entries.size() >= (size_t)group.maximum || used >= group.maxTotal) { AfxMessageBox(UiLanguage::Text(L"此类型格子或物品总数量已达到该配方的上限。")); return TRUE; }
                entries.emplace_back();
                int visible = 0; for (const auto& g : Recipe().groups) if (g.side == group.side && Visible(g.when, Draft())) ++visible;
                const int columns = (std::max)(1, (std::min)(3, (448 / visible) / 79));
                m_groupPages[group.id] = ((int)entries.size() - 1) / (columns * 3);
            } else if (entries.size() > (size_t)(std::max)(1, group.minimum)) entries.pop_back();
            Rebuild(); return TRUE;
        }
    }
    if (id >= FieldBase && id < FieldBase + Recipe().fields.size() &&
        (notification == CBN_SELCHANGE || (notification == BN_CLICKED && Recipe().fields[id - FieldBase].type == "boolean"))) {
        SaveControls(); Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wp, lp);
}
int CDlgOther::Hit(CPoint point) const { for (size_t i = 0; i < m_slots.size(); ++i) if (m_slots[i].rect.PtInRect(point)) return (int)i; return -1; }
void CDlgOther::Select(const Slot& slot) {
    const auto& group = Recipe().groups[slot.group]; const auto kind = group.fluid ? RecipeFluid : RecipeItem;
    bool available = false;
    if (m_source) for (INT_PTR i = 0; i < m_source->GetSize(); ++i)
        if (RecipeKinds(m_catalog, std::wstring(m_source->GetAt(i).GetString())) & kind) { available = true; break; }
    if (!available) { AfxMessageBox(UiLanguage::Text(L"请先在主界面导入包含所需物品或流体的 jar 文件。"), MB_ICONINFORMATION); return; }
    SaveControls(); auto& e = EntryAt(slot); int limit = group.maxCount;
    if (group.totalByCount) {
        int64_t used = 0; for (const auto& other : Draft().groups[group.id]) if (&other != &e && !other.id.empty()) used += other.count;
        const int64_t remaining = (std::max)(int64_t(0), (int64_t)group.maxTotal - used);
        limit = (int)(std::min)((int64_t)limit, remaining);
    }
    if (limit < 1) { AfxMessageBox(UiLanguage::Text(L"此类型格子或物品总数量已达到该配方的上限。")); return; }
    CDlgSelect select(this); select.SetItemList(m_source); select.SetCatalog(m_catalog, kind);
    select.SetCountLimit(limit, group.fluid); select.SetChanceMode(group.chance);
    select.SetInitialSelection(e.id.c_str(), e.count); select.SetInitialDetails(kind, e.chance);
    if (select.DoModal() == IDOK) {
        e.id = select.GetSelectedItem().GetString(); e.count = select.GetSelectedCount(); e.chance = select.GetSelectedChance();
        UpdateTips(); InvalidateRect(slot.rect, FALSE);
    }
}
void CDlgOther::OnPaint() {
    CPaintDC dc(this); if (m_definitions.mods.empty()) return;
    m_ui.Arrow(&dc, m_ui.Rect(478, 194, 42, 58));
    for (size_t i = 0; i < m_slots.size(); ++i) {
        const auto& slot = m_slots[i]; auto& e = EntryAt(slot);
        m_ui.Slot(&dc, slot.rect, m_hover == (int)i, e.id.c_str(), e.count, m_textures);
        CRect label = slot.rect; int height = label.Height(); label.top = label.bottom + 3; label.bottom = label.top + height / 3;
        m_ui.Text(&dc, slot.label, label, false, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        if (Recipe().groups[slot.group].chance && !e.id.empty() && e.chance != 100) {
            CString chance; chance.Format(L"%.4g%%", e.chance); CRect rect = slot.rect; rect.bottom = rect.top + height / 3;
            int saved = dc.SaveDC(); dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
            CRect shadow = rect; shadow.OffsetRect(1, 1); dc.SetTextColor(RGB(55, 55, 55)); dc.DrawText(chance, shadow, DT_LEFT | DT_SINGLELINE);
            dc.SetTextColor(RGB(255, 255, 255)); dc.DrawText(chance, rect, DT_LEFT | DT_SINGLELINE); dc.RestoreDC(saved);
        }
    }
}
void CDlgOther::OnLButtonDown(UINT flags, CPoint point) { int n = Hit(point); if (n >= 0) Select(m_slots[n]); CDialogEx::OnLButtonDown(flags, point); }
void CDlgOther::OnRButtonDown(UINT flags, CPoint point) { int n = Hit(point); if (n >= 0) { EntryAt(m_slots[n]) = Entry(); UpdateTips(); InvalidateRect(m_slots[n].rect, FALSE); } CDialogEx::OnRButtonDown(flags, point); }
void CDlgOther::OnMouseMove(UINT flags, CPoint point) {
    int n = Hit(point); if (n != m_hover) { int previous = m_hover; m_hover = n; if (previous >= 0) InvalidateRect(m_slots[previous].rect, FALSE); if (n >= 0) InvalidateRect(m_slots[n].rect, FALSE); }
    if (!m_tracking) { TRACKMOUSEEVENT event = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0}; m_tracking = TrackMouseEvent(&event) != FALSE; }
    CDialogEx::OnMouseMove(flags, point);
}
void CDlgOther::OnMouseLeave() { m_tracking = false; if (m_hover >= 0) InvalidateRect(m_slots[m_hover].rect, FALSE); m_hover = -1; CDialogEx::OnMouseLeave(); }
BOOL CDlgOther::PreTranslateMessage(MSG* message) { m_ui.Relay(message); return CDialogEx::PreTranslateMessage(message); }
void CDlgOther::OnOK() {
    if (m_definitions.mods.empty()) return;
    SaveControls(); Json body; std::wstring error;
    if (!Build(Recipe(), m_version, Draft(), body, error, m_locale)) { AfxMessageBox(error.c_str(), MB_ICONINFORMATION); return; }
    m_script = Script(body).c_str(); CDialogEx::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgOther, CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
