#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgRecipeChanges.h"
#include "CDlgSelect.h"
#include <set>

using namespace RecipeChanges;
namespace { enum Controls { ModId=2800, RecipeId }; }
IMPLEMENT_DYNAMIC(CDlgRecipeChanges,CDialogEx)
CDlgRecipeChanges::CDlgRecipeChanges(bool remove,CWnd* parent) : CDialogEx(remove ? IDD_DLGREMOVE : IDD_DLGMODIFY,parent),
    m_remove(remove), m_method(remove ? Method::RemoveInput : Method::ReplaceInput) {}
BEGIN_MESSAGE_MAP(CDlgRecipeChanges,CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
CWnd* CDlgRecipeChanges::Control(LPCTSTR cls,const CString& text,UINT id,int x,int y,int w,int h,DWORD style) {
    auto control=std::make_unique<CWnd>();
    if(!control->Create(cls,text,WS_CHILD | WS_VISIBLE | style,m_ui.Rect(x,y,w,h),this,id)) return nullptr;
    control->SetFont(GetFont()); auto result=control.get(); m_controls.push_back(std::move(control)); return result;
}
void CDlgRecipeChanges::Label(const CString& text,int x,int y,int w) { Control(_T("STATIC"),text,0,x,y,w,28); }
BOOL CDlgRecipeChanges::OnInitDialog() {
    CDialogEx::OnInitDialog(); m_ui.Initialize(this,760,460);
    m_radios=m_remove ? std::vector<UINT>{IDC_RADIORMINPUT,IDC_RADIORMOUTPUT,IDC_RADIOMOD,IDC_RADIORECIPEID} :
        std::vector<UINT>{IDC_RADIOMODIFYINPUT,IDC_RADIOMODIFYOUTPUT};
    for(size_t i=0;i<m_radios.size();++i) {
        auto radio=GetDlgItem(m_radios[i]);
        radio->ModifyStyle(WS_GROUP | WS_TABSTOP,i==0 ? WS_GROUP | WS_TABSTOP : 0);
        m_ui.Move(m_radios[i],22+(int)i*180,22,170,28);
        radio->SetFont(GetFont()); radio->SendMessage(BM_SETCHECK,i==0 ? BST_CHECKED : BST_UNCHECKED);
    }
    GetDlgItem(IDOK)->ModifyStyle(0,WS_GROUP);
    Rebuild(); GetDlgItem(m_radios[0])->SetFocus(); return FALSE;
}
void CDlgRecipeChanges::SaveControls() {
    CString text;
    if(GetDlgItem(ModId)) { GetDlgItemText(ModId,text); text.Trim(); Draft().mod=text.GetString(); }
    if(GetDlgItem(RecipeId)) { GetDlgItemText(RecipeId,text); text.Trim(); Draft().recipeId=text.GetString(); }
}
void CDlgRecipeChanges::AddSlot(bool replacement,int x,int y,const CString& label) {
    m_slots.push_back({replacement,m_ui.Rect(x,y,82,82),label}); Label(label,x-30,y-38,210);
}
void CDlgRecipeChanges::Rebuild() {
    m_rebuilding=true; m_ui.ClearTips();
    for(auto& control:m_controls) if(control->GetSafeHwnd()) control->DestroyWindow();
    m_controls.clear(); m_slots.clear(); m_hover=-1;
    auto& d=Draft();
    if(m_method==Method::RemoveMod) {
        Label(_T("模组 ID"),22,120,130);
        auto combo=Control(_T("COMBOBOX"),_T(""),ModId,155,116,580,220,CBS_DROPDOWN | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL);
        if(combo) {
            std::set<std::wstring> mods;
            if(m_source) for(INT_PTR i=0;i<m_source->GetSize();++i) {
                std::wstring id=m_source->GetAt(i).GetString(); auto colon=id.find(L':');
                if(colon!=std::wstring::npos && colon) mods.insert(id.substr(0,colon));
            }
            for(const auto& mod:mods) combo->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(mod.c_str()));
            combo->SetWindowText(d.mod.c_str());
        }
        Label(_T("选择已导入的命名空间，或手动填写，例如 farmersdelight。"),22,175,713);
        Label(_T("移除该命名空间下的全部配方。"),22,230,713);
    } else if(m_method==Method::RemoveId) {
        Label(_T("配方 ID"),22,120,130);
        Control(_T("EDIT"),CString(d.recipeId.c_str()),RecipeId,155,116,580,28,WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
        Label(_T("例如 minecraft:iron_ingot_from_smelting_raw_iron。"),22,175,713);
        Label(_T("填写完整配方 ID；它可能与产物 ID 不同。"),22,230,713);
    } else if(m_remove) {
        AddSlot(false,330,145,m_method==Method::RemoveInput ? _T("匹配输入物品") : _T("匹配输出物品"));
        Label(m_method==Method::RemoveInput ? _T("移除所有使用该物品作为原料的配方。") : _T("移除所有产出该物品的配方。"),22,270,713);
    } else {
        AddSlot(false,165,145,_T("替换前的物品")); AddSlot(true,535,145,_T("替换后的物品"));
        Label(_T("限定配方 ID"),22,278,150);
        Control(_T("EDIT"),CString(d.recipeId.c_str()),RecipeId,175,275,560,28,WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
        Label(_T("留空时替换所有匹配的配方；填写后仅修改指定配方。"),22,315,713);
    }
    Label(_T("物品格：左键选择，右键清空，悬停查看物品 ID。"),22,365,713);
    // Group the resource radios separately from parameters and action buttons.
    if(!m_controls.empty()) m_controls.front()->ModifyStyle(0,WS_GROUP);
    CWnd* previous=nullptr;
    for(auto id:m_radios) { auto radio=GetDlgItem(id); radio->SetWindowPos(previous ? previous : &CWnd::wndTop,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); previous=radio; }
    for(auto& control:m_controls) { control->SetWindowPos(previous,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); previous=control.get(); }
    GetDlgItem(IDOK)->SetWindowPos(previous,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    GetDlgItem(IDCANCEL)->SetWindowPos(GetDlgItem(IDOK),0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    UpdateTips(); m_rebuilding=false; Invalidate(TRUE);
}
void CDlgRecipeChanges::UpdateTips() {
    m_ui.ClearTips();
    for(size_t i=0;i<m_slots.size();++i) m_ui.Tip(i+1,m_slots[i].rect,m_slots[i].label,CString(EntryAt(m_slots[i]).c_str()),1);
}
BOOL CDlgRecipeChanges::OnCommand(WPARAM wp,LPARAM lp) {
    if(!m_rebuilding && HIWORD(wp)==BN_CLICKED) for(size_t i=0;i<m_radios.size();++i) if(LOWORD(wp)==m_radios[i]) {
        SaveControls(); m_method=static_cast<Method>((m_remove ? 0 : 4)+(int)i);
        for(size_t j=0;j<m_radios.size();++j) GetDlgItem(m_radios[j])->SendMessage(BM_SETCHECK,i==j ? BST_CHECKED : BST_UNCHECKED);
        Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wp,lp);
}
int CDlgRecipeChanges::Hit(CPoint point) const { for(size_t i=0;i<m_slots.size();++i) if(m_slots[i].rect.PtInRect(point)) return (int)i; return -1; }
void CDlgRecipeChanges::Select(const Slot& slot) {
    bool available=false;
    if(m_source) for(INT_PTR i=0;i<m_source->GetSize();++i)
        if(RecipeKinds(m_catalog,std::wstring(m_source->GetAt(i).GetString())) & RecipeItem) { available=true; break; }
    if(!available) { AfxMessageBox(_T("请先在主界面导入包含物品的 jar 文件。"),MB_ICONINFORMATION); return; }
    SaveControls(); auto& item=EntryAt(slot);
    CDlgSelect select(this); select.SetItemList(m_source); select.SetCatalog(m_catalog,RecipeItem);
    select.SetCountLimit(1); select.SetChanceMode(false); select.SetInitialSelection(CString(item.c_str()),1);
    if(select.DoModal()==IDOK) { item=select.GetSelectedItem().GetString(); UpdateTips(); InvalidateRect(slot.rect,FALSE); }
}
void CDlgRecipeChanges::OnPaint() {
    CPaintDC dc(this); if(!m_remove) m_ui.Arrow(&dc,m_ui.Rect(285,150,190,70));
    for(size_t i=0;i<m_slots.size();++i) m_ui.Slot(&dc,m_slots[i].rect,m_hover==(int)i,CString(EntryAt(m_slots[i]).c_str()),1,m_textures);
}
void CDlgRecipeChanges::OnLButtonDown(UINT flags,CPoint point) { int n=Hit(point); if(n>=0) Select(m_slots[n]); CDialogEx::OnLButtonDown(flags,point); }
void CDlgRecipeChanges::OnRButtonDown(UINT flags,CPoint point) { int n=Hit(point); if(n>=0) { EntryAt(m_slots[n]).clear(); UpdateTips(); InvalidateRect(m_slots[n].rect,FALSE); } CDialogEx::OnRButtonDown(flags,point); }
void CDlgRecipeChanges::OnMouseMove(UINT flags,CPoint point) {
    int n=Hit(point); if(n!=m_hover) { int previous=m_hover; m_hover=n; if(previous>=0) InvalidateRect(m_slots[previous].rect,FALSE); if(n>=0) InvalidateRect(m_slots[n].rect,FALSE); }
    if(!m_tracking) { TRACKMOUSEEVENT event={sizeof(TRACKMOUSEEVENT),TME_LEAVE,m_hWnd,0}; m_tracking=TrackMouseEvent(&event)!=FALSE; }
    CDialogEx::OnMouseMove(flags,point);
}
void CDlgRecipeChanges::OnMouseLeave() { m_tracking=false; if(m_hover>=0) InvalidateRect(m_slots[m_hover].rect,FALSE); m_hover=-1; CDialogEx::OnMouseLeave(); }
BOOL CDlgRecipeChanges::PreTranslateMessage(MSG* message) { m_ui.Relay(message); return CDialogEx::PreTranslateMessage(message); }
void CDlgRecipeChanges::OnOK() {
    SaveControls(); std::wstring script,error;
    if(!Build(m_method,Draft(),script,error)) { AfxMessageBox(CString(error.c_str()),MB_ICONINFORMATION); return; }
    m_script=script.c_str(); CDialogEx::OnOK();
}
