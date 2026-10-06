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
    BookTab, ToolMode, ToolTag, SoundMode, SoundId, RecipeGroup,
    CustomTime, HeatMode, MoldMode, MoldId, ColorMode, ColorId, DyeEdit, DyeSpin, InputPage, OutputPage,
    AddInputItem, AddInputFluid, RemoveInputItem, RemoveInputFluid,
    AddOutputItem, AddOutputFluid, RemoveOutputItem, RemoveOutputFluid };
const wchar_t* Tabs[]={L"",L"meals",L"drinks",L"misc"};
const wchar_t* Sounds[]={L"",L"minecraft:item.axe.strip",L"minecraft:entity.sheep.shear"};
const wchar_t* Colors[]={L"white",L"orange",L"magenta",L"light_blue",L"yellow",L"lime",L"pink",L"gray",L"light_gray",L"cyan",L"purple",L"blue",L"brown",L"green",L"red",L"black"};
const wchar_t* Molds[]={L"createdieselgenerators:bar",L"createdieselgenerators:bowl",L"createdieselgenerators:chain",L"createdieselgenerators:lines"};
CString Integer(int value) { CString text; text.Format(_T("%d"),value); return text; }
CString Decimal(double value) { CString text; text.Format(_T("%.12g"),value); return text; }
}
IMPLEMENT_DYNAMIC(CDlgOther,CDialogEx)
CDlgOther::CDlgOther(CWnd* parent) : CDialogEx(IDD_DLGOTHER,parent) {
    m_drafts[0].inputs.resize(6); m_drafts[0].outputs.resize(1);
    m_drafts[1].inputs.resize(1); m_drafts[1].outputs.resize(4);
    for(int i=0;i<CreateAddonRecipe::MethodCount;++i) m_addonDrafts[i]=CreateAddonRecipe::MakeDraft(static_cast<CreateAddonRecipe::Method>(i));
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
    const wchar_t* mods[]={L"农夫乐事",L"机械动力：龙+",L"机械动力：柴油动力"};
    for(int i=0;i<3;++i) { int n=m_mods.AddString(mods[i]); m_mods.SetItemData(n,i); }
    m_mods.SetCurSel(m_mods.FindStringExact(-1,mods[0])); m_ui.Move(IDC_COMBOOTHER,82,16,215,150);
    m_methods.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,m_ui.Rect(400,16,220,150),this,RecipeType);
    m_methods.SetFont(GetFont()); PopulateMethods();
    m_versions.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,m_ui.Rect(748,16,130,150),this,TargetVersion);
    m_versions.SetFont(GetFont()); m_versions.AddString(_T("1.20.1")); m_versions.AddString(_T("1.21.1")); m_versions.SetCurSel(m_version==Version::Minecraft1201 ? 0 : 1);
    Rebuild(); return TRUE;
}
Entry& CDlgOther::EntryAt(const Slot& slot) {
    if(m_mod) {
        auto& d=AddonDraft();
        switch(slot.group) {
        case Group::Input: return d.inputs[slot.index]; case Group::Output: return d.outputs[slot.index];
        case Group::FluidInput: return d.fluidInputs[slot.index]; default: return d.fluidOutputs[slot.index];
        }
    }
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
        if((m_mod || m_method==Method::Cutting) && m_slots[i].group==Group::Output && !e.id.empty()) {
            CString probability; probability.Format(_T("（概率 %.12g%%）"),e.chance); label+=probability;
        }
        if(e.kind==RecipeFluid && !e.id.empty()) label+=_T("（mB）");
        m_ui.Tip(i+1,m_slots[i].rect,label,CString(e.id.c_str()),e.count);
    }
}
void CDlgOther::SaveControls() {
    if(m_mod) {
        auto& d=AddonDraft();
        if(GetDlgItem(CustomTime)) d.customTime=GetDlgItem(CustomTime)->SendMessage(BM_GETCHECK)==BST_CHECKED;
        if(GetDlgItem(TimeEdit)) { BOOL valid; UINT n=GetDlgItemInt(TimeEdit,&valid,FALSE); d.time=valid && n<=99999999 ? (int)n : 0; }
        if(GetDlgItem(HeatMode)) d.heat=(int)GetDlgItem(HeatMode)->SendMessage(CB_GETCURSEL);
        if(GetDlgItem(MoldMode)) { int n=(int)GetDlgItem(MoldMode)->SendMessage(CB_GETCURSEL); if(n>=0 && n<4) d.mold=Molds[n]; }
        if(GetDlgItem(MoldId) && GetDlgItem(MoldMode)->SendMessage(CB_GETCURSEL)==4) { CString text; GetDlgItemText(MoldId,text); text.Trim(); d.mold=text.GetString(); }
        if(GetDlgItem(ColorMode)) { int n=(int)GetDlgItem(ColorMode)->SendMessage(CB_GETCURSEL); if(n>=0 && n<16) d.color=std::wstring(L"minecraft:")+Colors[n]; }
        if(GetDlgItem(ColorId) && GetDlgItem(ColorMode)->SendMessage(CB_GETCURSEL)==16) { CString text; GetDlgItemText(ColorId,text); text.Trim(); d.color=text.GetString(); }
        if(GetDlgItem(DyeEdit)) { BOOL valid; UINT n=GetDlgItemInt(DyeEdit,&valid,FALSE); d.dyeAmount=valid && n<=1000 ? (int)n : 0; }
        return;
    }
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
void CDlgOther::PopulateMethods() {
    m_methods.ResetContent();
    if(!m_mod) { m_methods.AddString(_T("厨锅烹饪")); m_methods.AddString(_T("砧板切割")); m_methods.SetCurSel((int)m_method); return; }
    int first=m_mod==1 ? 0 : 4, last=m_mod==1 ? 4 : CreateAddonRecipe::MethodCount;
    for(int i=first;i<last;++i) {
        int n=m_methods.AddString(CreateAddonRecipe::GetInfo(static_cast<CreateAddonRecipe::Method>(i)).name);
        m_methods.SetItemData(n,i); if(i==m_addonMethods[m_mod-1]) m_methods.SetCurSel(n);
    }
}
void CDlgOther::RebuildAddon() {
    const auto method=AddonMethod(); const auto& info=CreateAddonRecipe::GetInfo(method); auto& d=AddonDraft();
    m_inputPage=(std::max)(0,(std::min)(m_inputPage,((int)d.inputs.size()-1)/9));
    m_outputPage=(std::max)(0,(std::min)(m_outputPage,((int)d.outputs.size()-1)/9));
    auto buttons=[&](int x,bool output) {
        int items=output ? info.itemsOut : info.itemsIn, fluids=output ? info.fluidsOut : info.fluidsIn;
        if(items>1) {
            Control(_T("BUTTON"),_T("+物品"),output ? AddOutputItem : AddInputItem,x,52,80,28,BS_PUSHBUTTON | WS_TABSTOP);
            Control(_T("BUTTON"),_T("−物品"),output ? RemoveOutputItem : RemoveInputItem,x+172,52,80,28,BS_PUSHBUTTON | WS_TABSTOP);
        }
        if(fluids>1) {
            Control(_T("BUTTON"),_T("+流体"),output ? AddOutputFluid : AddInputFluid,x+86,52,80,28,BS_PUSHBUTTON | WS_TABSTOP);
            Control(_T("BUTTON"),_T("−流体"),output ? RemoveOutputFluid : RemoveInputFluid,x+258,52,80,28,BS_PUSHBUTTON | WS_TABSTOP);
        }
    };
    buttons(22,false); buttons(480,true);
    CString title;
    if(info.itemsIn) {
        title.Format(_T("原料（物品总数 ≤%d）"),info.itemsIn); Label(title,22,85,245);
        if(d.inputs.size()>9) {
            std::vector<CString> pages; for(int i=0;i<((int)d.inputs.size()+8)/9;++i) { CString text; text.Format(_T("第 %d 页"),i+1); pages.push_back(text); }
            Combo(InputPage,270,85,100,pages,m_inputPage);
        }
        int first=m_inputPage*9;
        for(int i=first;i<(std::min)(first+9,(int)d.inputs.size());++i) {
            title.Format(_T("原料 %d"),i+1); int n=i-first; AddSlot(Group::Input,i,22+(n%3)*82,125+(n/3)*82,58,title);
        }
    } else Label(_T("流体原料"),22,85,260);
    for(int i=0;i<(int)d.fluidInputs.size();++i) {
        title.Format(_T("流体原料 %d"),i+1); AddSlot(Group::FluidInput,i,info.itemsIn ? 295 : 60,125+i*100,58,title);
    }
    Label(info.itemsOut ? _T("产物（物品可设置概率）") : _T("流体产物（按格子顺序输出）"),480,85,270);
    if(d.outputs.size()>9) {
        std::vector<CString> pages; for(int i=0;i<((int)d.outputs.size()+8)/9;++i) { CString text; text.Format(_T("第 %d 页"),i+1); pages.push_back(text); }
        Combo(OutputPage,750,85,128,pages,m_outputPage);
    }
    for(int i=m_outputPage*9;i<(std::min)(m_outputPage*9+9,(int)d.outputs.size());++i) {
        int local=i-m_outputPage*9;
        title.Format(_T("产物 %d"),i+1); AddSlot(Group::Output,i,480+(local%3)*82,125+(local/3)*82,58,title);
    }
    for(int i=0;i<(int)d.fluidOutputs.size();++i) {
        title.Format(_T("流体产物 %d"),i+1);
        AddSlot(Group::FluidOutput,i,info.itemsOut ? 750 : 480+(i%3)*82,125+(info.itemsOut ? i : i/3)*100,58,title);
    }
    if(info.mold) {
        Label(_T("模具"),22,389,112); int chosen=4; for(int i=0;i<4;++i) if(d.mold==Molds[i]) chosen=i;
        Combo(MoldMode,135,385,285,{_T("棒状铸模"),_T("碗铸模"),_T("锁链铸模"),_T("波纹铸模"),_T("自定义模具 ID")},chosen);
        if(chosen==4) { Label(_T("模具 ID"),22,424,112); Edit(CString(d.mold.c_str()),MoldId,135,420,285); }
        Label(_T("模具作为工具使用，不放入消耗原料。"),480,389,398);
    }
    if(method==CreateAddonRecipe::Coloring) {
        Label(_T("染料颜色"),22,389,112); int chosen=16;
        for(int i=0;i<16;++i) if(d.color==std::wstring(L"minecraft:")+Colors[i]) chosen=i;
        Combo(ColorMode,135,385,285,{_T("白色"),_T("橙色"),_T("品红色"),_T("淡蓝色"),_T("黄色"),_T("黄绿色"),_T("粉红色"),_T("灰色"),_T("淡灰色"),_T("青色"),_T("紫色"),_T("蓝色"),_T("棕色"),_T("绿色"),_T("红色"),_T("黑色"),_T("自定义染料变种 ID")},chosen);
        if(chosen==16) { Label(_T("颜色 ID"),22,424,112); Edit(CString(d.color.c_str()),ColorId,135,420,285); }
        Label(_T("染料用量"),480,389,112); Edit(Integer(d.dyeAmount),DyeEdit,590,385,110,true); Spin(DyeEdit,DyeSpin,680,385,d.dyeAmount);
        GetDlgItem(DyeSpin)->SendMessage(UDM_SETRANGE32,1,1000); Label(_T("mB"),715,389,65);
        Label(_T("默认 32 mB；颜色例如 minecraft:red。"),480,424,398);
    }
    if(info.time) {
        auto check=Control(_T("BUTTON"),_T("指定处理时间"),CustomTime,22,454,190,28,BS_AUTOCHECKBOX | WS_TABSTOP);
        if(check) check->SendMessage(BM_SETCHECK,d.customTime ? BST_CHECKED : BST_UNCHECKED);
        Edit(Integer(d.time),TimeEdit,215,454,120,true); Spin(TimeEdit,TimeSpin,315,454,d.time);
        GetDlgItem(TimeEdit)->EnableWindow(d.customTime); GetDlgItem(TimeSpin)->EnableWindow(d.customTime);
        Label(_T("tick"),345,458,65);
    }
    if(info.heat) { Label(_T("热量条件"),480,458,112); Combo(HeatMode,590,454,288,{_T("无需加热"),_T("加热"),_T("超级加热")},d.heat); }
    Label(info.time ? _T("左键选择，右键清空；流体单位 mB。未指定处理时间时使用模组默认值。") : _T("左键选择，右键清空；物品产物可设置概率，流体单位 mB。"),22,502,856);
}
void CDlgOther::ChangeAddonSlots(Group group,bool add) {
    auto& d=AddonDraft(); const auto& info=CreateAddonRecipe::GetInfo(AddonMethod());
    auto& entries=group==Group::Input ? d.inputs : group==Group::Output ? d.outputs : group==Group::FluidInput ? d.fluidInputs : d.fluidOutputs;
    int limit=group==Group::Input ? info.itemsIn : group==Group::Output ? info.itemsOut : group==Group::FluidInput ? info.fluidsIn : info.fluidsOut;
    if(add) {
        int used=(int)entries.size();
        if(group==Group::Input) { used=0; for(const auto& e:entries) used+=(std::max)(1,e.count); }
        if(used>=limit) { AfxMessageBox(_T("此类型格子或物品总数量已达到该配方的上限。"),MB_ICONINFORMATION); return; }
        Entry entry; entry.kind=group==Group::FluidInput || group==Group::FluidOutput ? RecipeFluid : RecipeItem; entries.push_back(entry);
    } else if(entries.size()>1) entries.pop_back();
    if(group==Group::Input) m_inputPage=((int)entries.size()-1)/9;
    if(group==Group::Output) m_outputPage=((int)entries.size()-1)/9;
}
void CDlgOther::Rebuild() {
    m_rebuilding=true; m_ui.ClearTips();
    for(auto& control:m_controls) if(control->GetSafeHwnd()) control->DestroyWindow();
    m_controls.clear(); m_slots.clear(); m_hover=-1;
    Label(_T("模组"),22,20,55); Label(_T("配方类型"),315,20,85); Label(_T("目标版本"),650,20,95);
    if(m_mod) { RebuildAddon(); UpdateTips(); m_rebuilding=false; Invalidate(TRUE); return; }
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
    if(notification==CBN_SELCHANGE && (id==IDC_COMBOOTHER || id==RecipeType || id==TargetVersion || id==ToolMode || id==SoundMode || id==MoldMode || id==ColorMode || id==InputPage || id==OutputPage)) {
        SaveControls();
        if(id==IDC_COMBOOTHER) { m_mod=(int)m_mods.GetItemData(m_mods.GetCurSel()); m_inputPage=m_outputPage=0; PopulateMethods(); }
        if(id==RecipeType) {
            if(m_mod) m_addonMethods[m_mod-1]=(int)m_methods.GetItemData(m_methods.GetCurSel());
            else m_method=m_methods.GetCurSel()==0 ? Method::Cooking : Method::Cutting;
            m_inputPage=m_outputPage=0;
        }
        if(id==InputPage) m_inputPage=(int)GetDlgItem(InputPage)->SendMessage(CB_GETCURSEL);
        if(id==OutputPage) m_outputPage=(int)GetDlgItem(OutputPage)->SendMessage(CB_GETCURSEL);
        if(id==TargetVersion) m_version=m_versions.GetCurSel()==0 ? Version::Minecraft1201 : Version::Minecraft1211;
        // Remember custom choices even when their initially empty edit is shown.
        if(m_mod && id==MoldMode && GetDlgItem(MoldMode)->SendMessage(CB_GETCURSEL)==4 && !GetDlgItem(MoldId)) AddonDraft().mold=L"";
        if(m_mod && id==ColorMode && GetDlgItem(ColorMode)->SendMessage(CB_GETCURSEL)==16 && !GetDlgItem(ColorId)) AddonDraft().color=L"";
        Rebuild(); return TRUE;
    }
    if(m_mod && notification==BN_CLICKED && id>=AddInputItem && id<=RemoveOutputFluid) {
        SaveControls();
        switch(id) {
        case AddInputItem: ChangeAddonSlots(Group::Input,true); break;
        case RemoveInputItem: ChangeAddonSlots(Group::Input,false); break;
        case AddInputFluid: ChangeAddonSlots(Group::FluidInput,true); break;
        case RemoveInputFluid: ChangeAddonSlots(Group::FluidInput,false); break;
        case AddOutputItem: ChangeAddonSlots(Group::Output,true); break;
        case RemoveOutputItem: ChangeAddonSlots(Group::Output,false); break;
        case AddOutputFluid: ChangeAddonSlots(Group::FluidOutput,true); break;
        case RemoveOutputFluid: ChangeAddonSlots(Group::FluidOutput,false); break;
        }
        Rebuild(); return TRUE;
    }
    if(id==CustomTime && notification==BN_CLICKED) { SaveControls(); Rebuild(); return TRUE; }
    return CDialogEx::OnCommand(wp,lp);
}
int CDlgOther::Hit(CPoint point) const { for(size_t i=0;i<m_slots.size();++i) if(m_slots[i].rect.PtInRect(point)) return (int)i; return -1; }
void CDlgOther::Select(const Slot& slot) {
    if(!m_source || !m_source->GetSize()) { AfxMessageBox(_T("请先在主界面导入 jar 文件。"),MB_ICONINFORMATION); return; }
    SaveControls(); auto& e=EntryAt(slot); bool output=slot.group==Group::Output;
    bool fluid=slot.group==Group::FluidInput || slot.group==Group::FluidOutput;
    RecipeEntryKind kind=fluid ? RecipeFluid : RecipeItem;
    CDlgSelect select(this); select.SetItemList(m_source); select.SetCatalog(m_catalog,kind);
    select.SetCountLimit(output ? 64 : m_mod && !fluid ? (std::min)(64,CreateAddonRecipe::GetInfo(AddonMethod()).itemsIn) : 1);
    select.SetChanceMode(output && (m_mod || m_method==Method::Cutting));
    select.SetInitialSelection(CString(e.id.c_str()),e.count); select.SetInitialDetails(kind,e.chance);
    if(select.DoModal()==IDOK) { e.id=select.GetSelectedItem().GetString(); e.count=select.GetSelectedCount(); e.chance=select.GetSelectedChance(); e.kind=kind; UpdateTips(); InvalidateRect(slot.rect,FALSE); }
}
void CDlgOther::OnPaint() {
    CPaintDC dc(this);
    m_ui.Arrow(&dc,m_mod ? m_ui.Rect(385,145,75,58) : m_ui.Rect(350,145,125,72));
    for(size_t i=0;i<m_slots.size();++i) {
        auto& slot=m_slots[i]; auto& e=EntryAt(slot);
        m_ui.Slot(&dc,slot.rect,m_hover==(int)i,CString(e.id.c_str()),e.count,m_textures);
        CRect label=slot.rect; int height=label.Height(); label.top=label.bottom+3; label.bottom=label.top+height/3;
        m_ui.Text(&dc,slot.label,label,false,DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        if((m_mod || m_method==Method::Cutting) && slot.group==Group::Output && !e.id.empty() && e.chance!=100) {
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
    bool valid=m_mod ? CreateAddonRecipe::Build(AddonMethod(),m_version,AddonDraft(),script,error) : Build(m_method,m_version,Draft(),script,error);
    if(!valid) { AfxMessageBox(CString(error.c_str()),MB_ICONINFORMATION); return; }
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
