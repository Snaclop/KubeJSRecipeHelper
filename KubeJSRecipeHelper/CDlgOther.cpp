#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgOther.h"
#include "CDlgSelect.h"
#include <cmath>
#include <cwchar>
#include <limits>

using namespace FarmersDelightRecipe;
namespace {
enum Controls { RecipeType=2300, TargetVersion, TimeEdit, TimeSpin, ExperienceEdit, ExperienceSpin,
    BookTab, ToolMode, ToolTag, SoundMode, SoundId, RecipeGroup };
const wchar_t* Tabs[]={L"",L"meals",L"drinks",L"misc"};
const wchar_t* Sounds[]={L"",L"minecraft:item.axe.strip",L"minecraft:entity.sheep.shear"};
CString Integer(int value) { CString text; text.Format(_T("%d"),value); return text; }
CString Decimal(double value) { CString text; text.Format(_T("%.12g"),value); return text; }
}
IMPLEMENT_DYNAMIC(CDlgOther,CDialogEx)
CDlgOther::CDlgOther(CWnd* parent) : CDialogEx(IDD_DLGOTHER,parent) {
    m_drafts[0].inputs.resize(6); m_drafts[0].outputs.resize(1);
    m_drafts[1].inputs.resize(1); m_drafts[1].outputs.resize(4);
}
CDlgOther::~CDlgOther() {}
void CDlgOther::DoDataExchange(CDataExchange* dx) { CDialogEx::DoDataExchange(dx); DDX_Control(dx,IDC_COMBOOTHER,m_mods); }
CWnd* CDlgOther::Control(LPCTSTR cls,const CString& text,UINT id,int x,int y,int w,int h,DWORD style) {
    auto wnd=std::make_unique<CWnd>();
    if(!wnd->Create(cls,text,WS_CHILD | WS_VISIBLE | style,m_ui.Rect(x,y,w,h),this,id)) return nullptr;
    wnd->SetFont(GetFont()); auto result=wnd.get(); m_controls.push_back(std::move(wnd)); return result;
}
void CDlgOther::Label(const CString& text,int x,int y,int w) { Control(_T("STATIC"),text,0,x,y,w,25); }
void CDlgOther::Edit(const CString& text,UINT id,int x,int y,int w,bool integer) {
    Control(_T("EDIT"),text,id,x,y,w,28,WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | (integer ? ES_NUMBER : 0));
}
void CDlgOther::Spin(UINT edit,UINT id,int x,int y,int value) {
    auto control=Control(UPDOWN_CLASS,_T(""),id,x,y,20,28,UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS);
    if(control) {
        control->SendMessage(UDM_SETBUDDY,reinterpret_cast<WPARAM>(GetDlgItem(edit)->GetSafeHwnd()));
        control->SendMessage(UDM_SETRANGE32,1,99999999); control->SendMessage(UDM_SETPOS32,0,value);
    }
}
CWnd* CDlgOther::Combo(UINT id,int x,int y,int w,const std::vector<CString>& choices,int selected) {
    auto control=Control(_T("COMBOBOX"),_T(""),id,x,y,w,170,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    if(control) { for(const auto& text:choices) control->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.GetString())); control->SendMessage(CB_SETCURSEL,selected); }
    return control;
}
BOOL CDlgOther::OnInitDialog() {
    CDialogEx::OnInitDialog(); m_rebuilding=true;
    m_ui.Initialize(this,900,580);
    m_mods.AddString(_T("农夫乐事")); m_mods.SetCurSel(0); m_ui.Move(IDC_COMBOOTHER,82,16,215,150);
    m_methods.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,m_ui.Rect(400,16,220,150),this,RecipeType);
    m_methods.SetFont(GetFont()); m_methods.AddString(_T("厨锅烹饪")); m_methods.AddString(_T("砧板切割")); m_methods.SetCurSel(0);
    m_versions.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,m_ui.Rect(748,16,130,150),this,TargetVersion);
    m_versions.SetFont(GetFont()); m_versions.AddString(_T("1.20.1")); m_versions.AddString(_T("1.21.1")); m_versions.SetCurSel(0);
    Rebuild(); return TRUE;
}
Entry& CDlgOther::EntryAt(const Slot& slot) {
    auto& d=Draft();
    switch(slot.group) {
    case Group::Input: return d.inputs[slot.index]; case Group::Output: return d.outputs[slot.index];
    case Group::Container: return d.container; default: return d.toolItem;
    }
}
void CDlgOther::AddSlot(Group group,int index,int x,int y,int size,const CString& label) {
    m_slots.push_back({group,index,m_ui.Rect(x,y,size,size),label});
}
void CDlgOther::UpdateTips() {
    m_ui.ClearTips();
    for(size_t i=0;i<m_slots.size();++i) {
        auto& e=EntryAt(m_slots[i]); CString label=m_slots[i].label;
        if(m_method==Method::Cutting && m_slots[i].group==Group::Output && !e.id.empty()) {
            CString probability; probability.Format(_T("（概率 %.12g%%）"),e.chance); label+=probability;
        }
        m_ui.Tip(i+1,m_slots[i].rect,label,CString(e.id.c_str()),e.count);
    }
}
void CDlgOther::SaveControls() {
    auto& d=Draft();
    if(GetDlgItem(TimeEdit)) { BOOL valid; UINT n=GetDlgItemInt(TimeEdit,&valid,FALSE); d.cookingTime=valid && n<=99999999 ? (int)n : 0; }
    if(GetDlgItem(ExperienceEdit)) {
        CString value; GetDlgItemText(ExperienceEdit,value); value.Trim(); wchar_t* end=nullptr;
        d.experience=wcstod(value.GetString(),&end);
        if(end==value.GetString() || *end) d.experience=std::numeric_limits<double>::quiet_NaN();
    }
    if(GetDlgItem(BookTab)) { int n=(int)GetDlgItem(BookTab)->SendMessage(CB_GETCURSEL); d.tab=n>=0 && n<4 ? Tabs[n] : L""; }
    if(GetDlgItem(ToolMode)) { int n=(int)GetDlgItem(ToolMode)->SendMessage(CB_GETCURSEL); if(n>=0 && n<4) d.tool=static_cast<Tool>(n); }
    if(GetDlgItem(ToolTag)) { CString value; GetDlgItemText(ToolTag,value); value.Trim(); if(!value.IsEmpty() && value[0]==_T('#')) value.Delete(0); d.toolTag=value.GetString(); }
    if(GetDlgItem(SoundMode)) {
        m_soundMode=(int)GetDlgItem(SoundMode)->SendMessage(CB_GETCURSEL);
        if(m_soundMode>=0 && m_soundMode<3) d.sound=Sounds[m_soundMode];
    }
    if(GetDlgItem(SoundId)) { CString value; GetDlgItemText(SoundId,value); value.Trim(); if(m_soundMode==3) d.sound=value.GetString(); }
    if(GetDlgItem(RecipeGroup)) { CString value; GetDlgItemText(RecipeGroup,value); d.group=value.GetString(); }
}
void CDlgOther::Rebuild() {
    m_rebuilding=true; m_ui.ClearTips();
    for(auto& control:m_controls) if(control->GetSafeHwnd()) control->DestroyWindow();
    m_controls.clear(); m_slots.clear(); m_hover=-1;
    Label(_T("模组"),22,20,55); Label(_T("配方类型"),315,20,85); Label(_T("目标版本"),650,20,95);
    auto& d=Draft();
    if(m_method==Method::Cooking) {
        Label(_T("原料（最多 6 格，每格消耗 1 个）"),22,90,470);
        for(int i=0;i<6;++i) { CString label; label.Format(_T("原料 %d"),i+1); AddSlot(Group::Input,i,50+(i%3)*90,140+(i/3)*105,72,label); }
        Label(_T("产物"),565,90,115); AddSlot(Group::Output,0,565,140,82,_T("产物"));
        Label(_T("容器（可选）"),715,90,165); AddSlot(Group::Container,0,715,140,82,_T("容器"));
        Label(_T("留空容器时，由产物决定是否需要容器。"),480,287,400);
        Label(_T("处理时间"),22,389,112); Edit(Integer(d.cookingTime),TimeEdit,135,385,120,true); Spin(TimeEdit,TimeSpin,235,385,d.cookingTime);
        Label(_T("tick"),270,389,65); Label(_T("经验"),375,389,80); Edit(Decimal(d.experience),ExperienceEdit,455,385,120);
        auto spin=Control(UPDOWN_CLASS,_T(""),ExperienceSpin,555,385,20,28,UDS_ALIGNRIGHT | UDS_ARROWKEYS);
        if(spin) { spin->SendMessage(UDM_SETBUDDY,reinterpret_cast<WPARAM>(GetDlgItem(ExperienceEdit)->GetSafeHwnd())); spin->SendMessage(UDM_SETRANGE32,0,1000000); }
        Label(_T("配方书"),635,389,95);
        int tab=0; for(int i=0;i<4;++i) if(d.tab==Tabs[i]) tab=i;
        Combo(BookTab,730,385,148,{_T("默认"),_T("餐点"),_T("饮品"),_T("杂项")},tab);
        Label(_T("默认 200 tick；20 tick = 1 秒。经验可填写小数。"),22,424,850);
    } else {
        Label(_T("原料（1 个）"),22,90,300); AddSlot(Group::Input,0,60,140,82,_T("原料"));
        Label(_T("产物（最多 4 格，分别设置概率）"),480,90,400);
        for(int i=0;i<4;++i) { CString label; label.Format(_T("产物 %d"),i+1); AddSlot(Group::Output,i,480+i*96,140,78,label); }
        Label(_T("工具类型"),22,304,112);
        Combo(ToolMode,135,300,285,{_T("刀具（标签）"),_T("斧（去皮工具／标签）"),_T("指定工具物品"),_T("自定义工具标签")},(int)d.tool);
        if(d.tool==Tool::Item) AddSlot(Group::Tool,0,60,350,72,_T("工具"));
        else if(d.tool==Tool::Tag) { Label(_T("标签 ID"),22,357,112); Edit(CString(d.toolTag.c_str()),ToolTag,135,353,285); Label(m_version==Version::Minecraft1201 ? _T("例如 forge:tools/knives，可带 #。") : _T("例如 c:tools/knife，可带 #。"),22,394,400); }
        else Label(d.tool==Tool::Knives ? _T("匹配任意刀具；标签随目标版本切换。") : _T("匹配可去皮的工具或 minecraft:axes。"),22,354,400);
        Label(_T("切割声音"),480,304,110);
        Combo(SoundMode,590,300,288,{_T("默认（由工具决定）"),_T("斧去皮"),_T("剪刀剪切"),_T("自定义声音 ID")},m_soundMode);
        if(m_soundMode==3) { Label(_T("声音 ID"),480,357,110); Edit(CString(d.sound.c_str()),SoundId,590,353,288); }
        Label(_T("产物概率是每个物品的掉落概率，支持小数。"),480,414,400);
    }
    Label(_T("配方分组"),22,459,112); Edit(CString(d.group.c_str()),RecipeGroup,135,455,743);
    Label(_T("左键选择，右键清空；悬停查看 ID、数量和概率。分组可留空。"),22,502,856);
    UpdateTips(); m_rebuilding=false; Invalidate(TRUE);
}
BOOL CDlgOther::OnCommand(WPARAM wp,LPARAM lp) {
    if(m_rebuilding) return CDialogEx::OnCommand(wp,lp);
    UINT id=LOWORD(wp),notification=HIWORD(wp);
    if(notification==CBN_SELCHANGE && (id==RecipeType || id==TargetVersion || id==ToolMode || id==SoundMode)) {
        SaveControls();
        if(id==RecipeType) m_method=m_methods.GetCurSel()==0 ? Method::Cooking : Method::Cutting;
        if(id==TargetVersion) m_version=m_versions.GetCurSel()==0 ? Version::Minecraft1201 : Version::Minecraft1211;
        Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wp,lp);
}
int CDlgOther::Hit(CPoint point) const { for(size_t i=0;i<m_slots.size();++i) if(m_slots[i].rect.PtInRect(point)) return (int)i; return -1; }
void CDlgOther::Select(const Slot& slot) {
    if(!m_source || !m_source->GetSize()) { AfxMessageBox(_T("请先在主界面导入 jar 文件。"),MB_ICONINFORMATION); return; }
    SaveControls(); auto& e=EntryAt(slot); bool output=slot.group==Group::Output;
    CDlgSelect select(this); select.SetItemList(m_source); select.SetCatalog(m_catalog,RecipeItem);
    select.SetCountLimit(output ? 64 : 1); select.SetChanceMode(output && m_method==Method::Cutting);
    select.SetInitialSelection(CString(e.id.c_str()),e.count); select.SetInitialDetails(RecipeItem,e.chance);
    if(select.DoModal()==IDOK) { e.id=select.GetSelectedItem().GetString(); e.count=select.GetSelectedCount(); e.chance=select.GetSelectedChance(); e.kind=RecipeItem; UpdateTips(); InvalidateRect(slot.rect,FALSE); }
}
void CDlgOther::OnPaint() {
    CPaintDC dc(this);
    m_ui.Arrow(&dc,m_ui.Rect(350,145,125,72));
    for(size_t i=0;i<m_slots.size();++i) {
        auto& slot=m_slots[i]; auto& e=EntryAt(slot);
        m_ui.Slot(&dc,slot.rect,m_hover==(int)i,CString(e.id.c_str()),e.count,m_textures);
        CRect label=slot.rect; int height=label.Height(); label.top=label.bottom+3; label.bottom=label.top+height/3;
        m_ui.Text(&dc,slot.label,label,false,DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        if(m_method==Method::Cutting && slot.group==Group::Output && !e.id.empty() && e.chance!=100) {
            CString probability; probability.Format(_T("%.4g%%"),e.chance); CRect rect=slot.rect; rect.bottom=rect.top+height/3;
            int saved=dc.SaveDC(); dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
            CRect shadow=rect; shadow.OffsetRect(1,1); dc.SetTextColor(RGB(55,55,55)); dc.DrawText(probability,shadow,DT_LEFT | DT_SINGLELINE);
            dc.SetTextColor(RGB(255,255,255)); dc.DrawText(probability,rect,DT_LEFT | DT_SINGLELINE); dc.RestoreDC(saved);
        }
    }
}
void CDlgOther::OnLButtonDown(UINT flags,CPoint point) { int n=Hit(point); if(n>=0) Select(m_slots[n]); CDialogEx::OnLButtonDown(flags,point); }
void CDlgOther::OnRButtonDown(UINT flags,CPoint point) { int n=Hit(point); if(n>=0) { EntryAt(m_slots[n])=Entry(); UpdateTips(); InvalidateRect(m_slots[n].rect,FALSE); } CDialogEx::OnRButtonDown(flags,point); }
void CDlgOther::OnMouseMove(UINT flags,CPoint point) {
    int n=Hit(point); if(n!=m_hover) { int previous=m_hover; m_hover=n; if(previous>=0) InvalidateRect(m_slots[previous].rect,FALSE); if(n>=0) InvalidateRect(m_slots[n].rect,FALSE); }
    if(!m_tracking) { TRACKMOUSEEVENT event={sizeof(TRACKMOUSEEVENT),TME_LEAVE,m_hWnd,0}; m_tracking=TrackMouseEvent(&event)!=FALSE; }
    CDialogEx::OnMouseMove(flags,point);
}
void CDlgOther::OnMouseLeave() { m_tracking=false; if(m_hover>=0) InvalidateRect(m_slots[m_hover].rect,FALSE); m_hover=-1; CDialogEx::OnMouseLeave(); }
void CDlgOther::OnExperienceSpin(NMHDR* header,LRESULT* result) {
    auto delta=reinterpret_cast<NMUPDOWN*>(header); CString text; GetDlgItemText(ExperienceEdit,text); double value=wcstod(text,nullptr);
    if(!std::isfinite(value)) value=0;
    value=(std::max)(0.0,(std::min)(1000000.0,value-delta->iDelta*0.1)); SetDlgItemText(ExperienceEdit,Decimal(value)); *result=1;
}
BOOL CDlgOther::PreTranslateMessage(MSG* message) { m_ui.Relay(message); return CDialogEx::PreTranslateMessage(message); }
void CDlgOther::OnOK() {
    SaveControls(); std::wstring script,error;
    if(!Build(m_method,m_version,Draft(),script,error)) { AfxMessageBox(CString(error.c_str()),MB_ICONINFORMATION); return; }
    m_script=script.c_str(); CDialogEx::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgOther,CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_NOTIFY(UDN_DELTAPOS,ExperienceSpin,&CDlgOther::OnExperienceSpin)
END_MESSAGE_MAP()
