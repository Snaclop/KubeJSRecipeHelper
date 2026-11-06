#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgRecipeChanges.h"
#include "CDlgSelect.h"
#include "Language.h"
#include <set>

using namespace RecipeChanges;
namespace { enum Controls { ModId=2800, RecipeId }; }
IMPLEMENT_DYNAMIC(CDlgRecipeChanges,CDialogEx)
CDlgRecipeChanges::CDlgRecipeChanges(bool bRemove,CWnd* pParent) : CDialogEx(bRemove ? IDD_DLGREMOVE : IDD_DLGMODIFY,pParent),
    m_bRemove(bRemove), m_eMethod(bRemove ? Method::RemoveInput : Method::ReplaceInput) {}
BEGIN_MESSAGE_MAP(CDlgRecipeChanges,CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
CWnd* CDlgRecipeChanges::Control(LPCTSTR pszClass,const CString& strText,UINT uId,int nX,int nY,int nWidth,int nHeight,DWORD dwStyle) {
    auto pControl=std::make_unique<CWnd>();
    if(!pControl->Create(pszClass,strText,WS_CHILD | WS_VISIBLE | dwStyle,m_ui.Rect(nX,nY,nWidth,nHeight),this,uId)) return nullptr;
    pControl->SetFont(GetFont()); auto pResult=pControl.get(); m_arrControls.push_back(std::move(pControl)); return pResult;
}
void CDlgRecipeChanges::Label(const CString& strText,int nX,int nY,int nWidth) { Control(_T("STATIC"),strText,0,nX,nY,nWidth,28); }
BOOL CDlgRecipeChanges::OnInitDialog() {
    CDialogEx::OnInitDialog();
    UiLanguage::ApplyDialog(*this);
    m_ui.Initialize(this,760,460);
    m_arrRadioIds=m_bRemove ? std::vector<UINT>{IDC_RADIORMINPUT,IDC_RADIORMOUTPUT,IDC_RADIOMOD,IDC_RADIORECIPEID} :
        std::vector<UINT>{IDC_RADIOMODIFYINPUT,IDC_RADIOMODIFYOUTPUT};
    for(size_t nIndex=0;nIndex<m_arrRadioIds.size();++nIndex) {
        auto pRadio=GetDlgItem(m_arrRadioIds[nIndex]);
        pRadio->ModifyStyle(WS_GROUP | WS_TABSTOP,nIndex==0 ? WS_GROUP | WS_TABSTOP : 0);
        m_ui.Move(m_arrRadioIds[nIndex],22+(int)nIndex*180,22,170,28);
        pRadio->SetFont(GetFont()); pRadio->SendMessage(BM_SETCHECK,nIndex==0 ? BST_CHECKED : BST_UNCHECKED);
    }
    GetDlgItem(IDOK)->ModifyStyle(0,WS_GROUP);
    Rebuild(); GetDlgItem(m_arrRadioIds[0])->SetFocus(); return FALSE;
}
void CDlgRecipeChanges::SaveControls() {
    CString strText;
    if(GetDlgItem(ModId)) { GetDlgItemText(ModId,strText); strText.Trim(); Draft().strMod=strText.GetString(); }
    if(GetDlgItem(RecipeId)) { GetDlgItemText(RecipeId,strText); strText.Trim(); Draft().strRecipeId=strText.GetString(); }
}
void CDlgRecipeChanges::AddSlot(bool bReplacement,int nX,int nY,const CString& strLabel) {
    m_arrSlots.push_back({bReplacement,m_ui.Rect(nX,nY,82,82),strLabel}); Label(strLabel,nX-30,nY-38,210);
}
void CDlgRecipeChanges::Rebuild() {
    m_bRebuilding=true; m_ui.ClearTips();
    for(auto& pControl:m_arrControls) if(pControl->GetSafeHwnd()) pControl->DestroyWindow();
    m_arrControls.clear(); m_arrSlots.clear(); m_nHoverIndex=-1;
    auto& draft=Draft();
    if(m_eMethod==Method::RemoveMod) {
        Label(UiLanguage::Text(L"模组 ID"),22,120,130);
        auto pCombo=Control(_T("COMBOBOX"),_T(""),ModId,155,116,580,220,CBS_DROPDOWN | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL);
        if(pCombo) {
            std::set<std::wstring> setMods;
            if(m_pItemSource) for(INT_PTR nIndex=0;nIndex<m_pItemSource->GetSize();++nIndex) {
                std::wstring strId=m_pItemSource->GetAt(nIndex).GetString(); auto nColon=strId.find(L':');
                if(nColon!=std::wstring::npos && nColon) setMods.insert(strId.substr(0,nColon));
            }
            for(const auto& strMod:setMods) pCombo->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(strMod.c_str()));
            pCombo->SetWindowText(draft.strMod.c_str());
        }
        Label(UiLanguage::Text(L"选择已导入的命名空间，或手动填写，例如 farmersdelight。"),22,175,713);
        Label(UiLanguage::Text(L"移除该命名空间下的全部配方。"),22,230,713);
    } else if(m_eMethod==Method::RemoveId) {
        Label(UiLanguage::Text(L"配方 ID"),22,120,130);
        Control(_T("EDIT"),CString(draft.strRecipeId.c_str()),RecipeId,155,116,580,28,WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
        Label(UiLanguage::Text(L"例如 minecraft:iron_ingot_from_smelting_raw_iron。"),22,175,713);
        Label(UiLanguage::Text(L"填写完整配方 ID；它可能与产物 ID 不同。"),22,230,713);
    } else if(m_bRemove) {
        AddSlot(false,330,145,m_eMethod==Method::RemoveInput ? UiLanguage::Text(L"匹配输入物品") : UiLanguage::Text(L"匹配输出物品"));
        Label(m_eMethod==Method::RemoveInput ? UiLanguage::Text(L"移除所有使用该物品作为原料的配方。") : UiLanguage::Text(L"移除所有产出该物品的配方。"),22,270,713);
    } else {
        AddSlot(false,165,145,UiLanguage::Text(L"替换前的物品")); AddSlot(true,535,145,UiLanguage::Text(L"替换后的物品"));
        Label(UiLanguage::Text(L"限定配方 ID"),22,278,150);
        Control(_T("EDIT"),CString(draft.strRecipeId.c_str()),RecipeId,175,275,560,28,WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
        Label(UiLanguage::Text(L"留空时替换所有匹配的配方；填写后仅修改指定配方。"),22,315,713);
    }
    Label(UiLanguage::Text(L"物品格：左键选择，右键清空，悬停查看物品 ID。"),22,365,713);
    // Group the resource radios separately from parameters and action buttons.
    if(!m_arrControls.empty()) m_arrControls.front()->ModifyStyle(0,WS_GROUP);
    CWnd* pPrevious=nullptr;
    for(auto uId:m_arrRadioIds) { auto pRadio=GetDlgItem(uId); pRadio->SetWindowPos(pPrevious ? pPrevious : &CWnd::wndTop,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); pPrevious=pRadio; }
    for(auto& pControl:m_arrControls) { pControl->SetWindowPos(pPrevious,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); pPrevious=pControl.get(); }
    GetDlgItem(IDOK)->SetWindowPos(pPrevious,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    GetDlgItem(IDCANCEL)->SetWindowPos(GetDlgItem(IDOK),0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    UpdateTips(); m_bRebuilding=false; Invalidate(TRUE);
}
void CDlgRecipeChanges::UpdateTips() {
    m_ui.ClearTips();
    for(size_t nIndex=0;nIndex<m_arrSlots.size();++nIndex) m_ui.Tip(nIndex+1,m_arrSlots[nIndex].rcRect,m_arrSlots[nIndex].strLabel,CString(EntryAt(m_arrSlots[nIndex]).c_str()),1);
}
BOOL CDlgRecipeChanges::OnCommand(WPARAM wParam,LPARAM lParam) {
    if(!m_bRebuilding && HIWORD(wParam)==BN_CLICKED) for(size_t nIndex=0;nIndex<m_arrRadioIds.size();++nIndex) if(LOWORD(wParam)==m_arrRadioIds[nIndex]) {
        SaveControls(); m_eMethod=static_cast<Method>((m_bRemove ? 0 : 4)+(int)nIndex);
        for(size_t nRadioIndex=0;nRadioIndex<m_arrRadioIds.size();++nRadioIndex) GetDlgItem(m_arrRadioIds[nRadioIndex])->SendMessage(BM_SETCHECK,nIndex==nRadioIndex ? BST_CHECKED : BST_UNCHECKED);
        Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wParam,lParam);
}
int CDlgRecipeChanges::Hit(CPoint ptPoint) const { for(size_t nIndex=0;nIndex<m_arrSlots.size();++nIndex) if(m_arrSlots[nIndex].rcRect.PtInRect(ptPoint)) return (int)nIndex; return -1; }
void CDlgRecipeChanges::Select(const Slot& slot) {
    const bool bAllowTags = !slot.bReplacement || m_eMethod == Method::ReplaceInput;
    bool bAvailable=false;
    if(m_pItemSource) for(INT_PTR nIndex=0;nIndex<m_pItemSource->GetSize();++nIndex)
        if(RecipeKinds(m_pCatalog,std::wstring(m_pItemSource->GetAt(nIndex).GetString())) & RecipeItem) { bAvailable=true; break; }
    if(!bAvailable && !bAllowTags) { AfxMessageBox(UiLanguage::Text(L"请先在主界面导入包含物品的 jar 文件。"),MB_ICONINFORMATION); return; }
    SaveControls(); auto& strItem=EntryAt(slot);
    CDlgSelect select(this); select.SetItemList(m_pItemSource); select.SetCatalog(m_pCatalog,RecipeItem);
    select.SetAllowTags(bAllowTags);
    select.SetCountLimit(1); select.SetChanceMode(false); select.SetInitialSelection(CString(strItem.c_str()),1);
    if(select.DoModal()==IDOK) { strItem=select.GetSelectedItem().GetString(); UpdateTips(); InvalidateRect(slot.rcRect,FALSE); }
}
void CDlgRecipeChanges::OnPaint() {
    CPaintDC dc(this); if(!m_bRemove) m_ui.Arrow(&dc,m_ui.Rect(285,150,190,70));
    for(size_t nIndex=0;nIndex<m_arrSlots.size();++nIndex) m_ui.Slot(&dc,m_arrSlots[nIndex].rcRect,m_nHoverIndex==(int)nIndex,CString(EntryAt(m_arrSlots[nIndex]).c_str()),1,m_pTextures);
}
void CDlgRecipeChanges::OnLButtonDown(UINT uFlags,CPoint ptPoint) { int nSlotIndex=Hit(ptPoint); if(nSlotIndex>=0) Select(m_arrSlots[nSlotIndex]); CDialogEx::OnLButtonDown(uFlags,ptPoint); }
void CDlgRecipeChanges::OnRButtonDown(UINT uFlags,CPoint ptPoint) { int nSlotIndex=Hit(ptPoint); if(nSlotIndex>=0) { EntryAt(m_arrSlots[nSlotIndex]).clear(); UpdateTips(); InvalidateRect(m_arrSlots[nSlotIndex].rcRect,FALSE); } CDialogEx::OnRButtonDown(uFlags,ptPoint); }
void CDlgRecipeChanges::OnMouseMove(UINT uFlags,CPoint ptPoint) {
    int nSlotIndex=Hit(ptPoint); if(nSlotIndex!=m_nHoverIndex) { int nPreviousHover=m_nHoverIndex; m_nHoverIndex=nSlotIndex; if(nPreviousHover>=0) InvalidateRect(m_arrSlots[nPreviousHover].rcRect,FALSE); if(nSlotIndex>=0) InvalidateRect(m_arrSlots[nSlotIndex].rcRect,FALSE); }
    if(!m_bTracking) { TRACKMOUSEEVENT event={sizeof(TRACKMOUSEEVENT),TME_LEAVE,m_hWnd,0}; m_bTracking=TrackMouseEvent(&event)!=FALSE; }
    CDialogEx::OnMouseMove(uFlags,ptPoint);
}
void CDlgRecipeChanges::OnMouseLeave() { m_bTracking=false; if(m_nHoverIndex>=0) InvalidateRect(m_arrSlots[m_nHoverIndex].rcRect,FALSE); m_nHoverIndex=-1; CDialogEx::OnMouseLeave(); }
BOOL CDlgRecipeChanges::PreTranslateMessage(MSG* pMessage) { m_ui.Relay(pMessage); return CDialogEx::PreTranslateMessage(pMessage); }
void CDlgRecipeChanges::OnOK() {
    SaveControls(); std::wstring strScript,strError;
    if(!Build(m_eMethod,Draft(),strScript,strError)) { AfxMessageBox(UiLanguage::Text(strError.c_str()),MB_ICONINFORMATION); return; }
    m_strScript=strScript.c_str(); CDialogEx::OnOK();
}
