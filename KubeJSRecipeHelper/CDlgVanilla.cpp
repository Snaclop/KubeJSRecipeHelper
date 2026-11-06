#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgVanilla.h"
#include "CDlgSelect.h"
#include "Language.h"

using namespace VanillaRecipe;
namespace { enum Controls { RecipeType = 2700, TimeEdit, TimeSpin }; }
IMPLEMENT_DYNAMIC(CDlgVanilla, CDialogEx)
CDlgVanilla::CDlgVanilla(CWnd* pParent) : CDialogEx(IDD_DLGVANILLA, pParent) {}
BEGIN_MESSAGE_MAP(CDlgVanilla, CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

CWnd* CDlgVanilla::Control(LPCTSTR pszClass, const CString& strText, UINT uId,
    int nX, int nY, int nWidth, int nHeight, DWORD dwStyle, bool bFixed) {
    auto pControl = std::make_unique<CWnd>();
    if (!pControl->Create(pszClass, strText, WS_CHILD | WS_VISIBLE | dwStyle, m_ui.Rect(nX, nY, nWidth, nHeight), this, uId)) return nullptr;
    pControl->SetFont(GetFont());
    auto pResult = pControl.get();
    (bFixed ? m_arrFixedControls : m_arrControls).push_back(std::move(pControl));
    return pResult;
}
void CDlgVanilla::Label(const CString& strText, int nX, int nY, int nWidth) {
    Control(_T("STATIC"), strText, 0, nX, nY, nWidth, 28);
}
BOOL CDlgVanilla::OnInitDialog() {
    CDialogEx::OnInitDialog();
    UiLanguage::ApplyDialog(*this);
    // 模板只提供窗口；所有控件都由代码创建，允许资源编辑器保留默认控件。
    while (CWnd* pChild = GetWindow(GW_CHILD)) pChild->DestroyWindow();
    m_ui.Initialize(this, 760, 500);
    Control(_T("STATIC"), UiLanguage::Text(L"合成方式"), 0, 22, 18, 110, 28, 0, true);
    if (!m_comboMethods.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        m_ui.Rect(140, 18, 300, 240), this, RecipeType)) {
        AfxMessageBox(UiLanguage::Text(L"无法创建合成方式选择框。"), MB_ICONERROR);
        EndDialog(IDCANCEL); return FALSE;
    }
    m_comboMethods.SetFont(GetFont());
    for (int nIndex = 0; nIndex < MethodCount; ++nIndex) {
        const int nItemIndex = m_comboMethods.AddString(UiLanguage::Text(GetInfo(static_cast<Method>(nIndex)).pName));
        m_comboMethods.SetItemData(nItemIndex, nIndex);
    }
    m_comboMethods.SetCurSel(0);
    Control(_T("BUTTON"), UiLanguage::Text(L"确定"), IDOK, 536, 456, 100, 30,
        WS_TABSTOP | BS_DEFPUSHBUTTON, true);
    Control(_T("BUTTON"), UiLanguage::Text(L"取消"), IDCANCEL, 650, 456, 90, 30, WS_TABSTOP, true);
    Rebuild();
    m_comboMethods.SetFocus();
    return FALSE;
}
Entry& CDlgVanilla::EntryAt(const Slot& slot) { return slot.bOutput ? Draft().output : Draft().arrInputs[slot.nIndex]; }
void CDlgVanilla::AddSlot(int nIndex, bool bOutput, int nX, int nY, int nSize, const CString& strLabel) {
    m_arrSlots.push_back({nIndex, bOutput, m_ui.Rect(nX, nY, nSize, nSize), strLabel});
    Label(strLabel, nX - 22, nY - 34, nSize + 44);
}
void CDlgVanilla::SaveControls() {
    if (IsCooking(m_eMethod) && GetDlgItem(TimeEdit)) {
        BOOL bValid = FALSE;
        const UINT uTime = GetDlgItemInt(TimeEdit, &bValid, FALSE);
        Draft().nTime = bValid && uTime <= 1000000 ? static_cast<int>(uTime) : 0;
    }
}
void CDlgVanilla::Rebuild() {
    m_bRebuilding = true;
    m_ui.ClearTips();
    for (auto& pControl : m_arrControls) if (pControl->GetSafeHwnd()) pControl->DestroyWindow();
    m_arrControls.clear(); m_arrSlots.clear(); m_nHoverIndex = -1;
    if (IsCrafting(m_eMethod)) {
        Label(UiLanguage::Text(L"合成网格（3 × 3）"), 89, 70, 260);
        for (int nRow = 0; nRow < 3; ++nRow)
            for (int nColumn = 0; nColumn < 3; ++nColumn)
                m_arrSlots.push_back({nRow * 3 + nColumn, false, m_ui.Rect(105 + nColumn * 67, 112 + nRow * 67, 58, 58), UiLanguage::Text(L"原料")});
        AddSlot(0, true, 566, 174, 70, UiLanguage::Text(L"产物"));
        m_rcArrow = m_ui.Rect(355, 174, 160, 70);
        Label(m_eMethod == Method::Shaped ? UiLanguage::Text(L"每格一个原料，按摆放位置生成配方。") :
            UiLanguage::Text(L"忽略摆放位置；原料总数最多 9 个。"), 22, 337, 700);
    } else if (m_eMethod == Method::Smithing) {
        AddSlot(0, false, 68, 174, 70, UiLanguage::Text(L"锻造模板"));
        AddSlot(1, false, 178, 174, 70, UiLanguage::Text(L"升级物品"));
        AddSlot(2, false, 288, 174, 70, UiLanguage::Text(L"材料"));
        AddSlot(0, true, 566, 174, 70, UiLanguage::Text(L"产物"));
        m_rcArrow = m_ui.Rect(395, 174, 120, 70);
        Label(UiLanguage::Text(L"模板、升级物品和材料各一个；产物数量可设置。"), 22, 337, 700);
    } else {
        AddSlot(0, false, 158, 140, 70, UiLanguage::Text(L"原料"));
        AddSlot(0, true, 566, 140, 70, UiLanguage::Text(L"产物"));
        m_rcArrow = m_ui.Rect(290, 140, 215, 70);
        if (IsCooking(m_eMethod)) {
            Label(UiLanguage::Text(L"处理时间"), 22, 285, 110);
            CString strValue; strValue.Format(_T("%d"), Draft().nTime);
            auto pEdit = Control(_T("EDIT"), strValue, TimeEdit, 140, 285, 110, 28,
                WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL);
            auto pSpin = Control(UPDOWN_CLASS, _T(""), TimeSpin, 230, 285, 20, 28,
                UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS);
            if (pSpin && pEdit) {
                pSpin->SendMessage(UDM_SETBUDDY, reinterpret_cast<WPARAM>(pEdit->GetSafeHwnd()));
                pSpin->SendMessage(UDM_SETRANGE32, 1, 1000000);
                pSpin->SendMessage(UDM_SETPOS32, 0, Draft().nTime);
            }
            Label(UiLanguage::Text(L"tick（20 tick = 1 秒）"), 275, 285, 420);
        }
        Label(UiLanguage::Text(L"原料数量固定为 1；产物数量可设置。"), 22, 337, 700);
    }
    Label(UiLanguage::Text(L"左键选择，右键清空；停留在格子上可查看 ID 和数量。"), 22, 389, 700);
    // 下拉框 → 参数控件 → 确定 → 取消，重建后也保持键盘导航顺序。
    CWnd* pPrevious = &m_comboMethods;
    for (auto& pControl : m_arrControls) {
        pControl->SetWindowPos(pPrevious, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        pPrevious = pControl.get();
    }
    GetDlgItem(IDOK)->SetWindowPos(pPrevious, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    GetDlgItem(IDCANCEL)->SetWindowPos(GetDlgItem(IDOK), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    UpdateTips(); m_bRebuilding = false; Invalidate(TRUE);
}
void CDlgVanilla::UpdateTips() {
    m_ui.ClearTips();
    for (size_t nIndex = 0; nIndex < m_arrSlots.size(); ++nIndex) {
        const auto& slot = m_arrSlots[nIndex]; const auto& entry = EntryAt(slot);
        const int nCount = !slot.bOutput && m_eMethod == Method::Shaped ? 1 : entry.nCount;
        m_ui.Tip(nIndex + 1, slot.rcRect, slot.strLabel, CString(entry.strId.c_str()), nCount);
    }
}
BOOL CDlgVanilla::OnCommand(WPARAM wParam, LPARAM lParam) {
    if (!m_bRebuilding && LOWORD(wParam) == RecipeType && HIWORD(wParam) == CBN_SELCHANGE) {
        SaveControls();
        const int nIndex = m_comboMethods.GetCurSel();
        if (nIndex != CB_ERR) {
            m_eMethod = static_cast<Method>(m_comboMethods.GetItemData(nIndex));
            Rebuild();
        }
        return TRUE;
    }
    return CDialogEx::OnCommand(wParam, lParam);
}
int CDlgVanilla::Hit(CPoint ptPoint) const {
    for (size_t nIndex = 0; nIndex < m_arrSlots.size(); ++nIndex) if (m_arrSlots[nIndex].rcRect.PtInRect(ptPoint)) return static_cast<int>(nIndex);
    return -1;
}
void CDlgVanilla::Select(const Slot& slot) {
    bool bAvailable = false;
    if (m_pItemSource) for (INT_PTR nIndex = 0; nIndex < m_pItemSource->GetSize(); ++nIndex)
        if (RecipeKinds(m_pCatalog, std::wstring(m_pItemSource->GetAt(nIndex).GetString())) & RecipeItem) { bAvailable = true; break; }
    if (!bAvailable && slot.bOutput) {
        AfxMessageBox(UiLanguage::Text(L"还没有可选的物品，请先在主界面点击“导入 jar 文件”。"), MB_ICONINFORMATION); return;
    }
    const int nLimit = slot.bOutput ? 64 : InputCountLimit(m_eMethod, Draft(), slot.nIndex);
    if (nLimit < 1) { AfxMessageBox(UiLanguage::Text(L"无序合成的原料总数已经达到 9 个。"), MB_ICONINFORMATION); return; }
    auto& entry = EntryAt(slot);
    CDlgSelect select(this);
    select.SetItemList(m_pItemSource); select.SetCatalog(m_pCatalog, RecipeItem);
    select.SetCountLimit(nLimit); select.SetChanceMode(false);
    select.SetAllowTags(!slot.bOutput);
    select.SetInitialSelection(CString(entry.strId.c_str()), entry.nCount);
    if (select.DoModal() == IDOK) {
        entry.strId = select.GetSelectedItem().GetString(); entry.nCount = select.GetSelectedCount();
        UpdateTips(); InvalidateRect(&slot.rcRect, FALSE);
    }
}
void CDlgVanilla::OnOK() {
    SaveControls(); std::wstring strError;
    if (!Validate(m_eMethod, Draft(), strError)) {
        AfxMessageBox(UiLanguage::Text(strError.c_str()), MB_ICONINFORMATION); return;
    }
    m_strScript = BuildScript(m_eMethod, Draft()).c_str();
    CDialogEx::OnOK();
}
void CDlgVanilla::OnPaint() {
    CPaintDC dc(this); m_ui.Arrow(&dc, m_rcArrow);
    for (size_t nIndex = 0; nIndex < m_arrSlots.size(); ++nIndex) {
        const auto& slot = m_arrSlots[nIndex]; const auto& entry = EntryAt(slot);
        const int nCount = !slot.bOutput && m_eMethod == Method::Shaped ? 1 : entry.nCount;
        m_ui.Slot(&dc, slot.rcRect, static_cast<int>(nIndex) == m_nHoverIndex, CString(entry.strId.c_str()), nCount, m_pTextures);
    }
}
void CDlgVanilla::OnLButtonDown(UINT uFlags, CPoint ptPoint) {
    const int nIndex = Hit(ptPoint); if (nIndex >= 0) Select(m_arrSlots[nIndex]);
    CDialogEx::OnLButtonDown(uFlags, ptPoint);
}
void CDlgVanilla::OnRButtonDown(UINT uFlags, CPoint ptPoint) {
    const int nIndex = Hit(ptPoint);
    if (nIndex >= 0) { EntryAt(m_arrSlots[nIndex]) = Entry(); UpdateTips(); InvalidateRect(&m_arrSlots[nIndex].rcRect, FALSE); }
    CDialogEx::OnRButtonDown(uFlags, ptPoint);
}
void CDlgVanilla::OnMouseMove(UINT uFlags, CPoint ptPoint) {
    const int nHoverIndex = Hit(ptPoint);
    if (nHoverIndex != m_nHoverIndex) {
        if (m_nHoverIndex >= 0) InvalidateRect(&m_arrSlots[m_nHoverIndex].rcRect, FALSE);
        m_nHoverIndex = nHoverIndex;
        if (m_nHoverIndex >= 0) InvalidateRect(&m_arrSlots[m_nHoverIndex].rcRect, FALSE);
    }
    if (!m_bTracking) {
        TRACKMOUSEEVENT event = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, GetSafeHwnd(), 0};
        m_bTracking = TrackMouseEvent(&event) != FALSE;
    }
    CDialogEx::OnMouseMove(uFlags, ptPoint);
}
void CDlgVanilla::OnMouseLeave() {
    m_bTracking = false;
    if (m_nHoverIndex >= 0) InvalidateRect(&m_arrSlots[m_nHoverIndex].rcRect, FALSE);
    m_nHoverIndex = -1; CDialogEx::OnMouseLeave();
}
BOOL CDlgVanilla::PreTranslateMessage(MSG* pMessage) {
    m_ui.Relay(pMessage); return CDialogEx::PreTranslateMessage(pMessage);
}
