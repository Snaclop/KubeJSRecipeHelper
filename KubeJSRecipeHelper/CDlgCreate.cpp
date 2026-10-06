#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgCreate.h"
#include "CDlgSelect.h"
#include <algorithm>

using namespace CreateRecipe;
namespace {
enum Controls { AddInput = 2000, AddInputFluid, RemoveInput, AddOutput, AddOutputFluid, RemoveOutput,
    PrevInput, NextInput, PrevOutput, NextOutput, TimeCheck, TimeEdit, TimeSpin, Heat,
    KeepHeld, Rows, Columns, GridApply, Mirrored, Loops, LoopsSpin,
    StepList, StepType, StepAdd, StepRemove, StepUp, StepDown, StepKeep, StepTimeCheck, StepTime, StepTimeSpin,
    TargetVersion };
const Method StepMethods[] = { Pressing, Cutting, Deploying, Filling };
}
IMPLEMENT_DYNAMIC(CDlgCreate, CDialogEx)
CDlgCreate::CDlgCreate(CWnd* parent) : CDialogEx(IDD_DLGCREATE, parent) {
    for (int i = 0; i < MethodCount; ++i) {
        auto& d = m_drafts[i];
        int count = i == Deploying ? 2 : 1;
        d.inputs.resize(count); d.outputs.resize(1);
        if (i == Filling) { Entry fluid; fluid.kind = RecipeFluid; d.inputs.push_back(fluid); }
        if (i == Emptying) { Entry fluid; fluid.kind = RecipeFluid; d.outputs.push_back(fluid); }
        if (i == SequencedAssembly) { d.steps.push_back(Step()); d.outputs[0].chance = 1; d.loops = DefaultLoops(m_version); }
    }
}
CDlgCreate::~CDlgCreate() {}
void CDlgCreate::DoDataExchange(CDataExchange* dx) {
    CDialogEx::DoDataExchange(dx); DDX_Control(dx, IDC_COMBOCREATE, m_methods);
}
CRect CDlgCreate::Rect(int x, int y, int w, int h) const {
    return CRect(static_cast<int>(x * m_scale), static_cast<int>(y * m_scale),
        static_cast<int>((x + w) * m_scale), static_cast<int>((y + h) * m_scale));
}
CWnd* CDlgCreate::Control(LPCTSTR cls, const CString& text, UINT id, int x, int y, int w, int h, DWORD style) {
    auto wnd = std::make_unique<CWnd>();
    if (!wnd->Create(cls, text, WS_CHILD | WS_VISIBLE | style, Rect(x,y,w,h), this, id)) return nullptr;
    wnd->SetFont(GetFont()); CWnd* result = wnd.get(); m_controls.push_back(std::move(wnd)); return result;
}
void CDlgCreate::Label(const CString& text, int x, int y, int w) { Control(_T("STATIC"), text, 0, x,y,w,25); }
void CDlgCreate::Button(const CString& text, UINT id, int x, int y, int w) { Control(_T("BUTTON"), text,id,x,y,w,29,WS_TABSTOP); }
void CDlgCreate::Edit(int value, UINT id, int x, int y, int w) {
    CString text; text.Format(_T("%d"),value); Control(_T("EDIT"),text,id,x,y,w,28,WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL);
}
void CDlgCreate::Check(const CString& text, UINT id, bool checked, int x, int y, int w) {
    auto control = Control(_T("BUTTON"),text,id,x,y,w,28,BS_AUTOCHECKBOX | WS_TABSTOP);
    if (control) control->SendMessage(BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED);
}
void CDlgCreate::Spin(UINT edit, UINT id, int x, int y, int value, int maximum) {
    auto control = Control(UPDOWN_CLASS,_T(""),id,x,y,20,28,UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS);
    if (control) { control->SendMessage(UDM_SETBUDDY,reinterpret_cast<WPARAM>(GetDlgItem(edit)->GetSafeHwnd()));
        control->SendMessage(UDM_SETRANGE32,1,maximum); control->SendMessage(UDM_SETPOS32,0,value); }
}
BOOL CDlgCreate::OnInitDialog() {
    CDialogEx::OnInitDialog();
    CClientDC dc(this); m_scale = dc.GetDeviceCaps(LOGPIXELSX) / 96.0;
    // Keep the complete editor within the current monitor's working area.
    MONITORINFO monitor = {sizeof(MONITORINFO)};
    GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &monitor);
    m_scale = (std::min)(m_scale, (std::min)((monitor.rcWork.right-monitor.rcWork.left-50)/900.0,
        (monitor.rcWork.bottom-monitor.rcWork.top-65)/610.0));
    CRect outer = Rect(0,0,900,610); AdjustWindowRectEx(&outer, GetStyle(), FALSE, GetExStyle());
    SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),SWP_NOMOVE | SWP_NOZORDER); CenterWindow();
    m_methods.ModifyStyle(CBS_SORT, 0);
    for (int i=0;i<MethodCount;++i) { int index=m_methods.AddString(GetInfo(static_cast<Method>(i)).name); m_methods.SetItemData(index,i); }
    for (int i=0;i<m_methods.GetCount();++i) if(m_methods.GetItemData(i)==Compacting) m_methods.SetCurSel(i);
    m_methods.MoveWindow(Rect(125,16,285,370));
    m_versions.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        Rect(615,16,265,130),this,TargetVersion);
    m_versions.SetFont(GetFont());
    m_versions.AddString(_T("Minecraft 1.20.1"));
    m_versions.AddString(_T("Minecraft 1.21.1"));
    m_versions.SetCurSel(m_version == Version::Minecraft1201 ? 0 : 1);
    m_versions.SetWindowPos(&m_methods,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
    GetDlgItem(IDOK)->MoveWindow(Rect(676,566,100,30)); GetDlgItem(IDCANCEL)->MoveWindow(Rect(790,566,90,30));
    m_tips.Create(this, TTS_ALWAYSTIP); m_tips.SetMaxTipWidth(500); m_tips.Activate(TRUE);
    Rebuild(); return TRUE;
}
Entry& CDlgCreate::EntryAt(const Slot& slot) {
    auto& d = Draft();
    switch(slot.group) {
    case Input: return d.inputs[slot.index]; case Output: return d.outputs[slot.index];
    case Grid: return d.grid[slot.index]; case Transition: return d.transition;
    default: return d.steps[m_step].secondary;
    }
}
void CDlgCreate::AddSlot(Group group,int index,int x,int y,int size,const CString& label) {
    Slot slot = {Rect(x,y,size,size),group,index,label}; m_slots.push_back(slot);
    const auto& e = EntryAt(slot); CString tip(e.id.c_str());
    if (tip.IsEmpty()) tip = label + _T("：左键选择，右键清空");
    else { CString detail; detail.Format(e.kind == RecipeFluid ? _T("\n%d mB") : _T("\n数量：%d"),e.count); tip+=detail;
        if (group == Output && e.kind == RecipeItem) { detail.Format(m_method == SequencedAssembly ? _T("\n权重：%.12g") : _T("\n概率：%.12g%%"),e.chance); tip+=detail; } }
    m_tips.AddTool(this,tip,slot.rect,static_cast<UINT_PTR>(m_slots.size()));
}
void CDlgCreate::PageSlots(Group group,std::vector<Entry>& entries,int page,int x,int y) {
    for (int i=page*12;i<(int)entries.size() && i<(page+1)*12;++i) {
        int local=i-page*12; CString label;
        label.Format(entries[i].kind == RecipeFluid ? _T("流体 %d") : _T("物品 %d"),i+1);
        if (m_method == Deploying && group == Input) label = i==0 ? _T("原料") : _T("手持物品");
        AddSlot(group,i,x+(local%6)*67,y+(local/6)*94,58,label);
    }
}
void CDlgCreate::SaveControls() {
    auto& d=Draft();
    auto number=[this](UINT id,int& value) { if (GetDlgItem(id)) { BOOL valid; UINT n=GetDlgItemInt(id,&valid,FALSE); value=valid && n<=99999999 ? (int)n : 0; } };
    auto check=[this](UINT id,bool& value) { if (GetDlgItem(id)) value=GetDlgItem(id)->SendMessage(BM_GETCHECK)==BST_CHECKED; };
    number(TimeEdit,d.time); check(TimeCheck,d.customTime); check(KeepHeld,d.keepHeld); check(Mirrored,d.mirrored);
    const int previousLoops = d.loops;
    number(Loops,d.loops);
    if (m_method == SequencedAssembly && d.loops != previousLoops) m_customLoops = true;
    if (m_method==MechanicalCrafting) {
        BOOL r,c; UINT rows=GetDlgItemInt(Rows,&r,FALSE),cols=GetDlgItemInt(Columns,&c,FALSE);
        if(r && c && rows>=1 && rows<=9 && cols>=1 && cols<=9) { d.rows=(int)rows; d.columns=(int)cols; }
    }
    if (GetDlgItem(Heat)) d.heat=(int)GetDlgItem(Heat)->SendMessage(CB_GETCURSEL);
    if (m_step>=0 && m_step<(int)d.steps.size()) {
        auto& s=d.steps[m_step]; check(StepKeep,s.keepHeld); check(StepTimeCheck,s.customTime); number(StepTime,s.time);
    }
}
void CDlgCreate::Rebuild() {
    m_rebuilding=true;
    for (size_t i=0;i<m_slots.size();++i) m_tips.DelTool(this,i+1);
    for (auto& c:m_controls) if (c->GetSafeHwnd()) c->DestroyWindow();
    m_controls.clear(); m_slots.clear(); m_hover=-1;
    Label(_T("处理方法"),22,20,100);
    Label(_T("目标版本"),480,20,120);
    auto& d=Draft(); const auto& info=GetInfo(m_method);
    if (m_method==MechanicalCrafting) {
        Label(_T("合成网格（每格一个物品）"),22,64,460);
        Label(_T("行"),22,101,26); Edit(d.rows,Rows,52,96,52); Label(_T("列"),115,101,26); Edit(d.columns,Columns,145,96,52);
        Button(_T("更新网格"),GridApply,210,96,110);
        for(int r=0;r<d.rows;++r) for(int c=0;c<d.columns;++c)
            AddSlot(Grid,r*9+c,22+c*41,140+r*41,38,_T(""));
        Label(_T("产物"),505,70); AddSlot(Output,0,505,105,70,_T("物品"));
        Check(_T("允许镜像合成"),Mirrored,d.mirrored,505,225);
        Label(_T("缩小网格后，范围外的格子会保留。"),430,310,445);
        Label(_T("只有当前网格范围会生成到脚本中。"),430,344,445);
    } else if(m_method==SequencedAssembly) {
        BuildSequence();
    } else {
        CString title; title.Format(_T("原料（物品 ≤%d，流体 ≤%d）"),info.itemsIn,info.fluidsIn); Label(title,22,70,420);
        title.Format(_T("产物（物品 ≤%d，流体 ≤%d）"),info.itemsOut,info.fluidsOut); Label(title,480,70,420);
        PageSlots(Input,d.inputs,m_inputPage,22,110); PageSlots(Output,d.outputs,m_outputPage,480,110);
        if(info.itemsIn>1 && m_method!=Deploying) Button(_T("+物品"),AddInput,22,315,83);
        if(info.fluidsIn && m_method!=Filling) Button(_T("+流体"),AddInputFluid,113,315,83);
        if(m_method==Mixing || m_method==Compacting) Button(_T("减一格"),RemoveInput,204,315,87);
        if(info.itemsOut>1) Button(_T("+物品"),AddOutput,480,315,83);
        if(info.fluidsOut && m_method!=Emptying) Button(_T("+流体"),AddOutputFluid,571,315,83);
        if(info.itemsOut>1) Button(_T("减一格"),RemoveOutput,662,315,87);
        if(d.inputs.size()>12) { Button(_T("上一页"),PrevInput,22,354,83); Button(_T("下一页"),NextInput,113,354,83); title.Format(_T("%d / %d"),m_inputPage+1,((int)d.inputs.size()+11)/12); Label(title,220,358,100); }
        if(d.outputs.size()>12) { Button(_T("上一页"),PrevOutput,480,354,83); Button(_T("下一页"),NextOutput,571,354,83); }
        int y=410;
        if(info.heat) {
            Label(_T("加热条件"),22,y,100);
            auto box=Control(_T("COMBOBOX"),_T(""),Heat,135,y-3,220,160,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
            for(auto text:{_T("无需加热"),_T("加热"),_T("超级加热")}) box->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
            box->SendMessage(CB_SETCURSEL,d.heat); y+=45;
        }
        if(info.time) {
            Check(_T("自定义处理时间"),TimeCheck,d.customTime,22,y,220);
            Edit(d.time,TimeEdit,250,y); Spin(TimeEdit,TimeSpin,330,y,d.time);
            GetDlgItem(TimeEdit)->EnableWindow(d.customTime); GetDlgItem(TimeSpin)->EnableWindow(d.customTime);
            Label(_T("tick（默认 100，20 tick = 1 秒）"),375,y+3,480);
        }
        if(m_method==Deploying) Check(_T("保留机械手持有的物品"),KeepHeld,d.keepHeld,22,y);
    }
    Label(_T("左键选择，右键清空；停留在格子上可查看 ID、数量和概率。"),22,535,855);
    m_rebuilding=false; Invalidate(TRUE);
}
void CDlgCreate::BuildSequence() {
    auto& d=Draft(); if(m_step<0 || m_step>=(int)d.steps.size()) m_step=d.steps.empty() ? -1 : 0;
    Label(_T("原料"),22,67,85); Label(_T("过渡物品"),124,67,140); Label(_T("产物（按权重抽取一个）"),480,67,410);
    AddSlot(Input,0,22,103,65,_T("原料")); AddSlot(Transition,0,132,103,65,_T("过渡物品"));
    PageSlots(Output,d.outputs,m_outputPage,480,103);
    Label(_T("循环次数"),22,213,112); Edit(d.loops,Loops,140,208,100); Spin(Loops,LoopsSpin,220,208,d.loops);
    Button(_T("+产物"),AddOutput,480,299,88); Button(_T("减一格"),RemoveOutput,576,299,88);
    if(d.outputs.size()>12) { Button(_T("上一页"),PrevOutput,675,299,88); Button(_T("下一页"),NextOutput,773,299,88); }
    Label(_T("步骤（按顺序执行）"),22,253,390);
    auto list=Control(_T("LISTBOX"),_T(""),StepList,22,287,375,202,WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP);
    for(size_t i=0;i<d.steps.size();++i) { CString text; text.Format(_T("%d. %s"),(int)i+1,GetInfo(d.steps[i].method).name); list->SendMessage(LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.GetString())); }
    list->SendMessage(LB_SETCURSEL,m_step);
    Button(_T("添加"),StepAdd,22,497,80); Button(_T("删除"),StepRemove,114,497,80); Button(_T("上移"),StepUp,206,497,80); Button(_T("下移"),StepDown,298,497,80);
    if(m_step<0) return;
    auto& step=d.steps[m_step];
    Label(_T("当前步骤"),430,353,120);
    auto box=Control(_T("COMBOBOX"),_T(""),StepType,550,345,305,185,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    for(int i=0;i<4;++i) { box->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(GetInfo(StepMethods[i]).name)); if(step.method==StepMethods[i]) box->SendMessage(CB_SETCURSEL,i); }
    if(step.method==Deploying || step.method==Filling) {
        AddSlot(Secondary,0,430,395,65,step.method==Deploying ? _T("工具/材料") : _T("流体"));
        if(step.method==Deploying) Check(_T("保留手持物品"),StepKeep,step.keepHeld,550,404,305);
    }
    if(step.method==Cutting) {
        Check(_T("自定义时间"),StepTimeCheck,step.customTime,430,401,220);
        Edit(step.time,StepTime,680,401,150); Spin(StepTime,StepTimeSpin,810,401,step.time);
        GetDlgItem(StepTime)->EnableWindow(step.customTime); GetDlgItem(StepTimeSpin)->EnableWindow(step.customTime);
        Label(_T("tick（默认 100）"),680,439,205);
    }
    Label(_T("各步骤自动使用过渡物品作为输入和输出。"),430,490,455);
}
void CDlgCreate::AddEntry(bool output,RecipeEntryKind kind) {
    auto& d=Draft(); auto& entries=output ? d.outputs : d.inputs; const auto& info=GetInfo(m_method);
    int total=0; for(const auto& e:entries) if(e.kind==kind) ++total;
    int limit=kind==RecipeFluid ? (output ? info.fluidsOut : info.fluidsIn) : (output ? info.itemsOut : info.itemsIn);
    if(m_method==SequencedAssembly && output) limit=256;
    if(total>=limit) { AfxMessageBox(_T("此类型格子已达到该方法的上限。")); return; }
    Entry entry; entry.kind=kind; if(m_method==SequencedAssembly) entry.chance=1;
    entries.push_back(entry); (output ? m_outputPage : m_inputPage)=((int)entries.size()-1)/12;
}
BOOL CDlgCreate::OnCommand(WPARAM wp,LPARAM lp) {
    UINT id=LOWORD(wp),notification=HIWORD(wp); if(m_rebuilding) return CDialogEx::OnCommand(wp,lp);
    if(id==TargetVersion && notification==CBN_SELCHANGE) {
        SaveControls();
        m_version = m_versions.GetCurSel()==0 ? Version::Minecraft1201 : Version::Minecraft1211;
        if(!m_customLoops) m_drafts[SequencedAssembly].loops = DefaultLoops(m_version);
        Rebuild(); return TRUE;
    }
    if(id==IDC_COMBOCREATE && notification==CBN_SELCHANGE) {
        SaveControls(); m_method=static_cast<Method>(m_methods.GetItemData(m_methods.GetCurSel())); m_inputPage=m_outputPage=0; m_step=-1; Rebuild(); return TRUE;
    }
    if(id>=AddInput && id<=StepTimeSpin && (notification==BN_CLICKED || notification==CBN_SELCHANGE || notification==LBN_SELCHANGE)) {
        SaveControls(); auto& d=Draft();
        switch(id) {
        case AddInput: AddEntry(false,RecipeItem); break; case AddInputFluid: AddEntry(false,RecipeFluid); break;
        case AddOutput: AddEntry(true,RecipeItem); break; case AddOutputFluid: AddEntry(true,RecipeFluid); break;
        case RemoveInput: if(d.inputs.size()>1) d.inputs.pop_back(); break;
        case RemoveOutput: if(d.outputs.size()>1) d.outputs.pop_back(); break;
        case PrevInput: --m_inputPage; break; case NextInput: ++m_inputPage; break;
        case PrevOutput: --m_outputPage; break; case NextOutput: ++m_outputPage; break;
        case GridApply: { BOOL r,c; UINT rows=GetDlgItemInt(Rows,&r,FALSE),cols=GetDlgItemInt(Columns,&c,FALSE);
            if(!r || !c || rows<1 || rows>9 || cols<1 || cols>9) { AfxMessageBox(_T("行数和列数须为 1–9。")); return TRUE; }
            d.rows=(int)rows; d.columns=(int)cols; break; }
        case StepList: m_step=(int)GetDlgItem(StepList)->SendMessage(LB_GETCURSEL); break;
        case StepType: if(m_step>=0) { int n=(int)GetDlgItem(StepType)->SendMessage(CB_GETCURSEL); if(n>=0 && n<4) {
            d.steps[m_step].method=StepMethods[n]; d.steps[m_step].secondary=Entry();
            d.steps[m_step].secondary.kind=n==3 ? RecipeFluid : RecipeItem;
        } } break;
        case StepAdd: d.steps.push_back(Step()); m_step=(int)d.steps.size()-1; break;
        case StepRemove: if(m_step>=0) { d.steps.erase(d.steps.begin()+m_step); m_step=(std::min)(m_step,(int)d.steps.size()-1); } break;
        case StepUp: if(m_step>0) { std::swap(d.steps[m_step],d.steps[m_step-1]); --m_step; } break;
        case StepDown: if(m_step>=0 && m_step+1<(int)d.steps.size()) { std::swap(d.steps[m_step],d.steps[m_step+1]); ++m_step; } break;
        default: break;
        }
        m_inputPage=max(0,min(m_inputPage,((int)d.inputs.size()-1)/12));
        m_outputPage=max(0,min(m_outputPage,((int)d.outputs.size()-1)/12)); Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wp,lp);
}
int CDlgCreate::Hit(CPoint point) const { for(size_t i=0;i<m_slots.size();++i) if(m_slots[i].rect.PtInRect(point)) return (int)i; return -1; }
void CDlgCreate::Select(const Slot& slot) {
    if (m_source == nullptr || m_source->GetSize() == 0) {
        AfxMessageBox(_T("还没有可选的物品，请先在主界面点击“导入 jar 文件”。"), MB_ICONINFORMATION);
        return;
    }
    SaveControls(); Entry& e=EntryAt(slot);
    CDlgSelect select(this); select.SetItemList(m_source); select.SetCatalog(m_catalog,e.kind);
    bool output=slot.group==Output;
    int count=output ? 64 : 1;
    if(slot.group==Input && (m_method==Mixing || m_method==Compacting)) {
        count=64; for(int i=0;i<(int)Draft().inputs.size();++i) if(i!=slot.index && Draft().inputs[i].kind==RecipeItem && !Draft().inputs[i].id.empty()) count-=Draft().inputs[i].count;
        if(count<1 && e.kind==RecipeItem) { AfxMessageBox(_T("物品原料总数已经达到 64。")); return; }
    }
    select.SetCountLimit(count); select.SetChanceMode(output && m_method!=MechanicalCrafting,m_method==SequencedAssembly);
    select.SetInitialSelection(CString(e.id.c_str()),e.count); select.SetInitialDetails(e.kind,e.chance);
    if(select.DoModal()==IDOK) { e.id=select.GetSelectedItem().GetString(); e.count=select.GetSelectedCount(); e.kind=select.GetSelectedKind(); e.chance=select.GetSelectedChance(); Rebuild(); }
}
void CDlgCreate::OnPaint() {
    CPaintDC dc(this); auto old=dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
    for(size_t i=0;i<m_slots.size();++i) {
        const auto& slot=m_slots[i]; auto& e=EntryAt(slot); CRect r=slot.rect;
        dc.FillSolidRect(r,(int)i==m_hover ? RGB(176,176,176) : RGB(139,139,139)); dc.Draw3dRect(r,RGB(55,55,55),RGB(255,255,255));
        if(!e.id.empty()) {
            if(!m_textures || !m_textures->Draw(&dc,r,CString(e.id.c_str()))) { CRect t=r; t.DeflateRect(3,3); dc.SetTextColor(RGB(30,30,30)); dc.DrawText(CString(e.id.c_str()),t,DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS); }
            CString value;
            if(e.kind==RecipeFluid) value.Format(_T("%d mB"),e.count); else if(e.count>1) value.Format(_T("%d"),e.count);
            CRect t=r; t.top=t.bottom-(int)(22*m_scale); t.DeflateRect(2,0); dc.SetTextColor(RGB(255,255,255)); dc.DrawText(value,t,DT_RIGHT | DT_SINGLELINE);
            if(slot.group==Output && e.kind==RecipeItem && (m_method==SequencedAssembly || e.chance!=100)) {
                value.Format(m_method==SequencedAssembly ? _T("w:%.4g") : _T("%.4g%%"),e.chance); CRect t=r; t.bottom=t.top+(int)(22*m_scale); dc.SetTextColor(RGB(255,255,255)); dc.DrawText(value,t,DT_LEFT | DT_SINGLELINE);
            }
        }
        CRect label=r; label.top=r.bottom+2; label.bottom=label.top+(int)(27*m_scale); dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
        dc.DrawText(slot.label,label,DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    dc.SelectObject(old);
}
void CDlgCreate::OnLButtonDown(UINT flags,CPoint point) { int n=Hit(point); if(n>=0) { Slot slot=m_slots[n]; Select(slot); } CDialogEx::OnLButtonDown(flags,point); }
void CDlgCreate::OnRButtonDown(UINT flags,CPoint point) { int n=Hit(point); if(n>=0) { auto& e=EntryAt(m_slots[n]); e.id.clear(); e.count=1; e.chance=m_method==SequencedAssembly ? 1 : 100; SaveControls(); Rebuild(); } CDialogEx::OnRButtonDown(flags,point); }
void CDlgCreate::OnMouseMove(UINT flags,CPoint point) {
    int n=Hit(point); if(n!=m_hover) { m_hover=n; Invalidate(FALSE); }
    if(!m_tracking) { TRACKMOUSEEVENT tme={sizeof(TRACKMOUSEEVENT),TME_LEAVE,m_hWnd,0}; m_tracking=TrackMouseEvent(&tme)!=FALSE; }
    CDialogEx::OnMouseMove(flags,point);
}
void CDlgCreate::OnMouseLeave() { m_tracking=false; m_hover=-1; Invalidate(FALSE); CDialogEx::OnMouseLeave(); }
BOOL CDlgCreate::PreTranslateMessage(MSG* msg) { m_tips.RelayEvent(msg); return CDialogEx::PreTranslateMessage(msg); }
void CDlgCreate::OnOK() {
    SaveControls();
    if(m_method==MechanicalCrafting) { // Apply typed dimensions even when the grid button was not pressed.
        BOOL r,c; UINT rows=GetDlgItemInt(Rows,&r,FALSE),cols=GetDlgItemInt(Columns,&c,FALSE);
        if(!r || !c || rows<1 || rows>9 || cols<1 || cols>9) { AfxMessageBox(_T("行数和列数须为 1–9。")); return; }
        Draft().rows=(int)rows; Draft().columns=(int)cols;
    }
    std::wstring script,error;
    if(!Build(m_method,Draft(),script,error,m_version)) { AfxMessageBox(CString(error.c_str()),MB_ICONINFORMATION); return; }
    m_script=script.c_str(); CDialogEx::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgCreate,CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
