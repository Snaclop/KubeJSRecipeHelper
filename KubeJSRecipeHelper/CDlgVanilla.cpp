#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgVanilla.h"
#include "CDlgSelect.h"

using namespace VanillaRecipe;
namespace { enum Controls { RecipeType = 2700, TimeEdit, TimeSpin }; }
IMPLEMENT_DYNAMIC(CDlgVanilla, CDialogEx)
CDlgVanilla::CDlgVanilla(CWnd* parent) : CDialogEx(IDD_DLGVANILLA, parent) {}
BEGIN_MESSAGE_MAP(CDlgVanilla, CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

CWnd* CDlgVanilla::Control(LPCTSTR cls, const CString& text, UINT id,
    int x, int y, int w, int h, DWORD style, bool fixed) {
    auto control = std::make_unique<CWnd>();
    if (!control->Create(cls, text, WS_CHILD | WS_VISIBLE | style, m_ui.Rect(x, y, w, h), this, id)) return nullptr;
    control->SetFont(GetFont());
    auto result = control.get();
    (fixed ? m_fixedControls : m_controls).push_back(std::move(control));
    return result;
}
void CDlgVanilla::Label(const CString& text, int x, int y, int w) {
    Control(_T("STATIC"), text, 0, x, y, w, 28);
}
BOOL CDlgVanilla::OnInitDialog() {
    CDialogEx::OnInitDialog();
    // 模板只提供窗口；所有控件都由代码创建，允许资源编辑器保留默认控件。
    while (CWnd* child = GetWindow(GW_CHILD)) child->DestroyWindow();
    m_ui.Initialize(this, 760, 500);
    Control(_T("STATIC"), _T("合成方式"), 0, 22, 18, 110, 28, 0, true);
    if (!m_methods.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        m_ui.Rect(140, 18, 300, 240), this, RecipeType)) {
        AfxMessageBox(_T("无法创建合成方式选择框。"), MB_ICONERROR);
        EndDialog(IDCANCEL); return FALSE;
    }
    m_methods.SetFont(GetFont());
    for (int i = 0; i < MethodCount; ++i) {
        const int index = m_methods.AddString(GetInfo(static_cast<Method>(i)).name);
        m_methods.SetItemData(index, i);
    }
    m_methods.SetCurSel(0);
    Control(_T("BUTTON"), _T("确定"), IDOK, 536, 456, 100, 30,
        WS_TABSTOP | BS_DEFPUSHBUTTON, true);
    Control(_T("BUTTON"), _T("取消"), IDCANCEL, 650, 456, 90, 30, WS_TABSTOP, true);
    Rebuild();
    m_methods.SetFocus();
    return FALSE;
}
Entry& CDlgVanilla::EntryAt(const Slot& slot) { return slot.output ? Draft().output : Draft().inputs[slot.index]; }
void CDlgVanilla::AddSlot(int index, bool output, int x, int y, int size, const CString& label) {
    m_slots.push_back({index, output, m_ui.Rect(x, y, size, size), label});
    Label(label, x - 22, y - 34, size + 44);
}
void CDlgVanilla::SaveControls() {
    if (IsCooking(m_method) && GetDlgItem(TimeEdit)) {
        BOOL valid = FALSE;
        const UINT time = GetDlgItemInt(TimeEdit, &valid, FALSE);
        Draft().time = valid && time <= 1000000 ? static_cast<int>(time) : 0;
    }
}
void CDlgVanilla::Rebuild() {
    m_rebuilding = true;
    m_ui.ClearTips();
    for (auto& control : m_controls) if (control->GetSafeHwnd()) control->DestroyWindow();
    m_controls.clear(); m_slots.clear(); m_hover = -1;
    if (IsCrafting(m_method)) {
        Label(_T("合成网格（3 × 3）"), 89, 70, 260);
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                m_slots.push_back({row * 3 + col, false, m_ui.Rect(105 + col * 67, 112 + row * 67, 58, 58), _T("原料")});
        AddSlot(0, true, 566, 174, 70, _T("产物"));
        m_arrow = m_ui.Rect(355, 174, 160, 70);
        Label(m_method == Method::Shaped ? _T("每格一个原料，按摆放位置生成配方。") :
            _T("忽略摆放位置；原料总数最多 9 个。"), 22, 337, 700);
    } else if (m_method == Method::Smithing) {
        AddSlot(0, false, 68, 174, 70, _T("锻造模板"));
        AddSlot(1, false, 178, 174, 70, _T("升级物品"));
        AddSlot(2, false, 288, 174, 70, _T("材料"));
        AddSlot(0, true, 566, 174, 70, _T("产物"));
        m_arrow = m_ui.Rect(395, 174, 120, 70);
        Label(_T("模板、升级物品和材料各一个；产物数量可设置。"), 22, 337, 700);
    } else {
        AddSlot(0, false, 158, 140, 70, _T("原料"));
        AddSlot(0, true, 566, 140, 70, _T("产物"));
        m_arrow = m_ui.Rect(290, 140, 215, 70);
        if (IsCooking(m_method)) {
            Label(_T("处理时间"), 22, 285, 110);
            CString value; value.Format(_T("%d"), Draft().time);
            auto edit = Control(_T("EDIT"), value, TimeEdit, 140, 285, 110, 28,
                WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL);
            auto spin = Control(UPDOWN_CLASS, _T(""), TimeSpin, 230, 285, 20, 28,
                UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS);
            if (spin && edit) {
                spin->SendMessage(UDM_SETBUDDY, reinterpret_cast<WPARAM>(edit->GetSafeHwnd()));
                spin->SendMessage(UDM_SETRANGE32, 1, 1000000);
                spin->SendMessage(UDM_SETPOS32, 0, Draft().time);
            }
            Label(_T("tick（20 tick = 1 秒）"), 275, 285, 420);
        }
        Label(_T("原料数量固定为 1；产物数量可设置。"), 22, 337, 700);
    }
    Label(_T("左键选择，右键清空；停留在格子上可查看 ID 和数量。"), 22, 389, 700);
    // 下拉框 → 参数控件 → 确定 → 取消，重建后也保持键盘导航顺序。
    CWnd* previous = &m_methods;
    for (auto& control : m_controls) {
        control->SetWindowPos(previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        previous = control.get();
    }
    GetDlgItem(IDOK)->SetWindowPos(previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    GetDlgItem(IDCANCEL)->SetWindowPos(GetDlgItem(IDOK), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    UpdateTips(); m_rebuilding = false; Invalidate(TRUE);
}
void CDlgVanilla::UpdateTips() {
    m_ui.ClearTips();
    for (size_t i = 0; i < m_slots.size(); ++i) {
        const auto& slot = m_slots[i]; const auto& entry = EntryAt(slot);
        const int count = !slot.output && m_method == Method::Shaped ? 1 : entry.count;
        m_ui.Tip(i + 1, slot.rect, slot.label, CString(entry.id.c_str()), count);
    }
}
BOOL CDlgVanilla::OnCommand(WPARAM wp, LPARAM lp) {
    if (!m_rebuilding && LOWORD(wp) == RecipeType && HIWORD(wp) == CBN_SELCHANGE) {
        SaveControls();
        const int index = m_methods.GetCurSel();
        if (index != CB_ERR) {
            m_method = static_cast<Method>(m_methods.GetItemData(index));
            Rebuild();
        }
        return TRUE;
    }
    return CDialogEx::OnCommand(wp, lp);
}
int CDlgVanilla::Hit(CPoint point) const {
    for (size_t i = 0; i < m_slots.size(); ++i) if (m_slots[i].rect.PtInRect(point)) return static_cast<int>(i);
    return -1;
}
void CDlgVanilla::Select(const Slot& slot) {
    bool available = false;
    if (m_source) for (INT_PTR i = 0; i < m_source->GetSize(); ++i)
        if (RecipeKinds(m_catalog, std::wstring(m_source->GetAt(i).GetString())) & RecipeItem) { available = true; break; }
    if (!available) {
        AfxMessageBox(_T("还没有可选的物品，请先在主界面点击“导入 jar 文件”。"), MB_ICONINFORMATION); return;
    }
    const int limit = slot.output ? 64 : InputCountLimit(m_method, Draft(), slot.index);
    if (limit < 1) { AfxMessageBox(_T("无序合成的原料总数已经达到 9 个。"), MB_ICONINFORMATION); return; }
    auto& entry = EntryAt(slot);
    CDlgSelect select(this);
    select.SetItemList(m_source); select.SetCatalog(m_catalog, RecipeItem);
    select.SetCountLimit(limit); select.SetChanceMode(false);
    select.SetInitialSelection(CString(entry.id.c_str()), entry.count);
    if (select.DoModal() == IDOK) {
        entry.id = select.GetSelectedItem().GetString(); entry.count = select.GetSelectedCount();
        UpdateTips(); InvalidateRect(&slot.rect, FALSE);
    }
}
void CDlgVanilla::OnOK() {
    SaveControls(); std::wstring error;
    if (!Validate(m_method, Draft(), error)) {
        AfxMessageBox(CString(error.c_str()), MB_ICONINFORMATION); return;
    }
    m_script = BuildScript(m_method, Draft()).c_str();
    CDialogEx::OnOK();
}
void CDlgVanilla::OnPaint() {
    CPaintDC dc(this); m_ui.Arrow(&dc, m_arrow);
    for (size_t i = 0; i < m_slots.size(); ++i) {
        const auto& slot = m_slots[i]; const auto& entry = EntryAt(slot);
        const int count = !slot.output && m_method == Method::Shaped ? 1 : entry.count;
        m_ui.Slot(&dc, slot.rect, static_cast<int>(i) == m_hover, CString(entry.id.c_str()), count, m_textures);
    }
}
void CDlgVanilla::OnLButtonDown(UINT flags, CPoint point) {
    const int index = Hit(point); if (index >= 0) Select(m_slots[index]);
    CDialogEx::OnLButtonDown(flags, point);
}
void CDlgVanilla::OnRButtonDown(UINT flags, CPoint point) {
    const int index = Hit(point);
    if (index >= 0) { EntryAt(m_slots[index]) = Entry(); UpdateTips(); InvalidateRect(&m_slots[index].rect, FALSE); }
    CDialogEx::OnRButtonDown(flags, point);
}
void CDlgVanilla::OnMouseMove(UINT flags, CPoint point) {
    const int hover = Hit(point);
    if (hover != m_hover) {
        if (m_hover >= 0) InvalidateRect(&m_slots[m_hover].rect, FALSE);
        m_hover = hover;
        if (m_hover >= 0) InvalidateRect(&m_slots[m_hover].rect, FALSE);
    }
    if (!m_tracking) {
        TRACKMOUSEEVENT event = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, GetSafeHwnd(), 0};
        m_tracking = TrackMouseEvent(&event) != FALSE;
    }
    CDialogEx::OnMouseMove(flags, point);
}
void CDlgVanilla::OnMouseLeave() {
    m_tracking = false;
    if (m_hover >= 0) InvalidateRect(&m_slots[m_hover].rect, FALSE);
    m_hover = -1; CDialogEx::OnMouseLeave();
}
BOOL CDlgVanilla::PreTranslateMessage(MSG* message) {
    m_ui.Relay(message); return CDialogEx::PreTranslateMessage(message);
}
