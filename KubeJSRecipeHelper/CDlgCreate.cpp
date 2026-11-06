#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "CDlgCreate.h"
#include "CDlgSelect.h"
#include "Language.h"
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
CDlgCreate::CDlgCreate(CWnd* pParent) : CDialogEx(IDD_DLGCREATE, pParent) {
    for (int nIndex = 0; nIndex < MethodCount; ++nIndex) {
        auto& draft = m_arrDrafts[nIndex];
        int nCount = nIndex == Deploying ? 2 : 1;
        draft.arrInputs.resize(nCount); draft.arrOutputs.resize(1);
        if (nIndex == Filling) { Entry fluid; fluid.eKind = RecipeFluid; draft.arrInputs.push_back(fluid); }
        if (nIndex == Emptying) { Entry fluid; fluid.eKind = RecipeFluid; draft.arrOutputs.push_back(fluid); }
        if (nIndex == SequencedAssembly) { draft.arrSteps.push_back(Step()); draft.arrOutputs[0].dChance = 1; draft.nLoops = DefaultLoops(m_eVersion); }
    }
}
CDlgCreate::~CDlgCreate() {}
void CDlgCreate::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX); DDX_Control(pDX, IDC_COMBOCREATE, m_comboMethods);
}
CRect CDlgCreate::Rect(int nX, int nY, int nWidth, int nHeight) const {
    return CRect(static_cast<int>(nX * m_dScale), static_cast<int>(nY * m_dScale),
        static_cast<int>((nX + nWidth) * m_dScale), static_cast<int>((nY + nHeight) * m_dScale));
}
CWnd* CDlgCreate::Control(LPCTSTR pszClass, const CString& strText, UINT uId, int nX, int nY, int nWidth, int nHeight, DWORD dwStyle) {
    auto pWindow = std::make_unique<CWnd>();
    if (!pWindow->Create(pszClass, strText, WS_CHILD | WS_VISIBLE | dwStyle, Rect(nX,nY,nWidth,nHeight), this, uId)) return nullptr;
    pWindow->SetFont(GetFont()); CWnd* pResult = pWindow.get(); m_arrControls.push_back(std::move(pWindow)); return pResult;
}
void CDlgCreate::Label(const CString& strText, int nX, int nY, int nWidth) { Control(_T("STATIC"), strText, 0, nX,nY,nWidth,25); }
void CDlgCreate::Button(const CString& strText, UINT uId, int nX, int nY, int nWidth) { Control(_T("BUTTON"), strText,uId,nX,nY,nWidth,29,WS_TABSTOP); }
void CDlgCreate::Edit(int nValue, UINT uId, int nX, int nY, int nWidth) {
    CString strText; strText.Format(_T("%d"),nValue); Control(_T("EDIT"),strText,uId,nX,nY,nWidth,28,WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL);
}
void CDlgCreate::Check(const CString& strText, UINT uId, bool bChecked, int nX, int nY, int nWidth) {
    auto pControl = Control(_T("BUTTON"),strText,uId,nX,nY,nWidth,28,BS_AUTOCHECKBOX | WS_TABSTOP);
    if (pControl) pControl->SendMessage(BM_SETCHECK, bChecked ? BST_CHECKED : BST_UNCHECKED);
}
void CDlgCreate::Spin(UINT uEditId, UINT uId, int nX, int nY, int nValue, int nMaximum) {
    auto pControl = Control(UPDOWN_CLASS,_T(""),uId,nX,nY,20,28,UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS);
    if (pControl) { pControl->SendMessage(UDM_SETBUDDY,reinterpret_cast<WPARAM>(GetDlgItem(uEditId)->GetSafeHwnd()));
        pControl->SendMessage(UDM_SETRANGE32,1,nMaximum); pControl->SendMessage(UDM_SETPOS32,0,nValue); }
}
BOOL CDlgCreate::OnInitDialog() {
    CDialogEx::OnInitDialog();
    UiLanguage::ApplyDialog(*this);
    CClientDC dc(this); m_dScale = dc.GetDeviceCaps(LOGPIXELSX) / 96.0;
    // Keep the complete editor within the current monitor's working area.
    MONITORINFO monitor = {sizeof(MONITORINFO)};
    GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &monitor);
    m_dScale = (std::min)(m_dScale, (std::min)((monitor.rcWork.right-monitor.rcWork.left-50)/900.0,
        (monitor.rcWork.bottom-monitor.rcWork.top-65)/610.0));
    CRect rcOuter = Rect(0,0,900,610); AdjustWindowRectEx(&rcOuter, GetStyle(), FALSE, GetExStyle());
    SetWindowPos(nullptr,0,0,rcOuter.Width(),rcOuter.Height(),SWP_NOMOVE | SWP_NOZORDER); CenterWindow();
    m_comboMethods.ModifyStyle(CBS_SORT, 0);
    for (int nIndex=0;nIndex<MethodCount;++nIndex) { int nItemIndex=m_comboMethods.AddString(UiLanguage::Text(GetInfo(static_cast<Method>(nIndex)).pName)); m_comboMethods.SetItemData(nItemIndex,nIndex); }
    for (int nIndex=0;nIndex<m_comboMethods.GetCount();++nIndex) if(m_comboMethods.GetItemData(nIndex)==Compacting) m_comboMethods.SetCurSel(nIndex);
    m_comboMethods.MoveWindow(Rect(125,16,285,370));
    m_comboVersions.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        Rect(615,16,265,130),this,TargetVersion);
    m_comboVersions.SetFont(GetFont());
    m_comboVersions.AddString(_T("Minecraft 1.20.1"));
    m_comboVersions.AddString(_T("Minecraft 1.21.1"));
    m_comboVersions.SetCurSel(m_eVersion == Version::Minecraft1201 ? 0 : 1);
    m_comboVersions.SetWindowPos(&m_comboMethods,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
    GetDlgItem(IDOK)->MoveWindow(Rect(676,566,100,30)); GetDlgItem(IDCANCEL)->MoveWindow(Rect(790,566,90,30));
    m_toolTips.Create(this, TTS_ALWAYSTIP); m_toolTips.SetMaxTipWidth(500); m_toolTips.Activate(TRUE);
    Rebuild(); return TRUE;
}
Entry& CDlgCreate::EntryAt(const Slot& slot) {
    auto& draft = Draft();
    switch(slot.eGroup) {
    case Input: return draft.arrInputs[slot.nIndex]; case Output: return draft.arrOutputs[slot.nIndex];
    case Grid: return draft.arrGrid[slot.nIndex]; case Transition: return draft.transition;
    default: return draft.arrSteps[m_nStepIndex].secondary;
    }
}
void CDlgCreate::AddSlot(Group eGroup,int nIndex,int nX,int nY,int nSize,const CString& strLabel) {
    Slot slot = {Rect(nX,nY,nSize,nSize),eGroup,nIndex,strLabel}; m_arrSlots.push_back(slot);
    const auto& entry = EntryAt(slot); CString strTip(entry.strId.c_str());
    if (strTip.IsEmpty()) strTip = strLabel + UiLanguage::Text(L"：左键选择，右键清空");
    else { CString strDetail; strDetail.Format(entry.eKind == RecipeFluid ? _T("\n%d mB") : UiLanguage::Text(L"\n数量：%d"),entry.nCount); strTip+=strDetail;
        if (eGroup == Output && entry.eKind == RecipeItem) { strDetail.Format(m_eMethod == SequencedAssembly ? UiLanguage::Text(L"\n权重：%.12g") : UiLanguage::Text(L"\n概率：%.12g%%"),entry.dChance); strTip+=strDetail; } }
    m_toolTips.AddTool(this,strTip,slot.rcRect,static_cast<UINT_PTR>(m_arrSlots.size()));
}
void CDlgCreate::PageSlots(Group eGroup,std::vector<Entry>& arrEntries,int nPage,int nX,int nY) {
    for (int nIndex=nPage*12;nIndex<(int)arrEntries.size() && nIndex<(nPage+1)*12;++nIndex) {
        int nLocalIndex=nIndex-nPage*12; CString strLabel;
        strLabel.Format(arrEntries[nIndex].eKind == RecipeFluid ? UiLanguage::Text(L"流体 %d") : UiLanguage::Text(L"物品 %d"),nIndex+1);
        if (m_eMethod == Deploying && eGroup == Input) strLabel = nIndex==0 ? UiLanguage::Text(L"原料") : UiLanguage::Text(L"手持物品");
        AddSlot(eGroup,nIndex,nX+(nLocalIndex%6)*67,nY+(nLocalIndex/6)*94,58,strLabel);
    }
}
void CDlgCreate::SaveControls() {
    auto& draft=Draft();
    auto readNumber=[this](UINT uId,int& nValue) { if (GetDlgItem(uId)) { BOOL bValid; UINT uNumber=GetDlgItemInt(uId,&bValid,FALSE); nValue=bValid && uNumber<=99999999 ? (int)uNumber : 0; } };
    auto readCheck=[this](UINT uId,bool& bValue) { if (GetDlgItem(uId)) bValue=GetDlgItem(uId)->SendMessage(BM_GETCHECK)==BST_CHECKED; };
    readNumber(TimeEdit,draft.nTime); readCheck(TimeCheck,draft.bCustomTime); readCheck(KeepHeld,draft.bKeepHeld); readCheck(Mirrored,draft.bMirrored);
    const int nPreviousLoops = draft.nLoops;
    readNumber(Loops,draft.nLoops);
    if (m_eMethod == SequencedAssembly && draft.nLoops != nPreviousLoops) m_bCustomLoops = true;
    if (m_eMethod==MechanicalCrafting) {
        BOOL bRowsValid,bColumnsValid; UINT uRows=GetDlgItemInt(Rows,&bRowsValid,FALSE),uColumns=GetDlgItemInt(Columns,&bColumnsValid,FALSE);
        if(bRowsValid && bColumnsValid && uRows>=1 && uRows<=9 && uColumns>=1 && uColumns<=9) { draft.nRows=(int)uRows; draft.nColumns=(int)uColumns; }
    }
    if (GetDlgItem(Heat)) draft.nHeat=(int)GetDlgItem(Heat)->SendMessage(CB_GETCURSEL);
    if (m_nStepIndex>=0 && m_nStepIndex<(int)draft.arrSteps.size()) {
        auto& step=draft.arrSteps[m_nStepIndex]; readCheck(StepKeep,step.bKeepHeld); readCheck(StepTimeCheck,step.bCustomTime); readNumber(StepTime,step.nTime);
    }
}
void CDlgCreate::Rebuild() {
    m_bRebuilding=true;
    for (size_t nIndex=0;nIndex<m_arrSlots.size();++nIndex) m_toolTips.DelTool(this,nIndex+1);
    for (auto& pControl:m_arrControls) if (pControl->GetSafeHwnd()) pControl->DestroyWindow();
    m_arrControls.clear(); m_arrSlots.clear(); m_nHoverIndex=-1;
    Label(UiLanguage::Text(L"处理方法"),22,20,100);
    Label(UiLanguage::Text(L"目标版本"),480,20,120);
    auto& draft=Draft(); const auto& info=GetInfo(m_eMethod);
    if (m_eMethod==MechanicalCrafting) {
        Label(UiLanguage::Text(L"合成网格（每格一个物品）"),22,64,460);
        Label(UiLanguage::Text(L"行"),22,101,26); Edit(draft.nRows,Rows,52,96,52); Label(UiLanguage::Text(L"列"),115,101,26); Edit(draft.nColumns,Columns,145,96,52);
        Button(UiLanguage::Text(L"更新网格"),GridApply,210,96,110);
        for(int nRow=0;nRow<draft.nRows;++nRow) for(int nColumn=0;nColumn<draft.nColumns;++nColumn)
            AddSlot(Grid,nRow*9+nColumn,22+nColumn*41,140+nRow*41,38,_T(""));
        Label(UiLanguage::Text(L"产物"),505,70); AddSlot(Output,0,505,105,70,UiLanguage::Text(L"物品"));
        Check(UiLanguage::Text(L"允许镜像合成"),Mirrored,draft.bMirrored,505,225);
        Label(UiLanguage::Text(L"缩小网格后，范围外的格子会保留。"),430,310,445);
        Label(UiLanguage::Text(L"只有当前网格范围会生成到脚本中。"),430,344,445);
    } else if(m_eMethod==SequencedAssembly) {
        BuildSequence();
    } else {
        CString strTitle; strTitle.Format(UiLanguage::Text(L"原料（物品 ≤%d，流体 ≤%d）"),info.nItemsIn,info.nFluidsIn); Label(strTitle,22,70,420);
        strTitle.Format(UiLanguage::Text(L"产物（物品 ≤%d，流体 ≤%d）"),info.nItemsOut,info.nFluidsOut); Label(strTitle,480,70,420);
        PageSlots(Input,draft.arrInputs,m_nInputPage,22,110); PageSlots(Output,draft.arrOutputs,m_nOutputPage,480,110);
        if(info.nItemsIn>1 && m_eMethod!=Deploying) Button(UiLanguage::Text(L"+物品"),AddInput,22,315,83);
        if(info.nFluidsIn && m_eMethod!=Filling) Button(UiLanguage::Text(L"+流体"),AddInputFluid,113,315,83);
        if(m_eMethod==Mixing || m_eMethod==Compacting) Button(UiLanguage::Text(L"减一格"),RemoveInput,204,315,87);
        if(info.nItemsOut>1) Button(UiLanguage::Text(L"+物品"),AddOutput,480,315,83);
        if(info.nFluidsOut && m_eMethod!=Emptying) Button(UiLanguage::Text(L"+流体"),AddOutputFluid,571,315,83);
        if(info.nItemsOut>1) Button(UiLanguage::Text(L"减一格"),RemoveOutput,662,315,87);
        if(draft.arrInputs.size()>12) { Button(UiLanguage::Text(L"上一页"),PrevInput,22,354,83); Button(UiLanguage::Text(L"下一页"),NextInput,113,354,83); strTitle.Format(_T("%d / %d"),m_nInputPage+1,((int)draft.arrInputs.size()+11)/12); Label(strTitle,220,358,100); }
        if(draft.arrOutputs.size()>12) { Button(UiLanguage::Text(L"上一页"),PrevOutput,480,354,83); Button(UiLanguage::Text(L"下一页"),NextOutput,571,354,83); }
        int nY=410;
        if(info.bHeat) {
            Label(UiLanguage::Text(L"加热条件"),22,nY,100);
            auto pCombo=Control(_T("COMBOBOX"),_T(""),Heat,135,nY-3,220,160,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
            for(auto strText:{UiLanguage::Text(L"无需加热"),UiLanguage::Text(L"加热"),UiLanguage::Text(L"超级加热")}) pCombo->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(strText));
            pCombo->SendMessage(CB_SETCURSEL,draft.nHeat); nY+=45;
        }
        if(info.bTime) {
            Check(UiLanguage::Text(L"自定义处理时间"),TimeCheck,draft.bCustomTime,22,nY,220);
            Edit(draft.nTime,TimeEdit,250,nY); Spin(TimeEdit,TimeSpin,330,nY,draft.nTime);
            GetDlgItem(TimeEdit)->EnableWindow(draft.bCustomTime); GetDlgItem(TimeSpin)->EnableWindow(draft.bCustomTime);
            Label(UiLanguage::Text(L"tick（默认 100，20 tick = 1 秒）"),375,nY+3,480);
        }
        if(m_eMethod==Deploying) Check(UiLanguage::Text(L"保留机械手持有的物品"),KeepHeld,draft.bKeepHeld,22,nY);
    }
    Label(UiLanguage::Text(L"左键选择，右键清空；停留在格子上可查看 ID、数量和概率。"),22,535,855);
    m_bRebuilding=false; Invalidate(TRUE);
}
void CDlgCreate::BuildSequence() {
    auto& draft=Draft(); if(m_nStepIndex<0 || m_nStepIndex>=(int)draft.arrSteps.size()) m_nStepIndex=draft.arrSteps.empty() ? -1 : 0;
    Label(UiLanguage::Text(L"原料"),22,67,85); Label(UiLanguage::Text(L"过渡物品"),124,67,140); Label(UiLanguage::Text(L"产物（按权重抽取一个）"),480,67,410);
    AddSlot(Input,0,22,103,65,UiLanguage::Text(L"原料")); AddSlot(Transition,0,132,103,65,UiLanguage::Text(L"过渡物品"));
    PageSlots(Output,draft.arrOutputs,m_nOutputPage,480,103);
    Label(UiLanguage::Text(L"循环次数"),22,213,112); Edit(draft.nLoops,Loops,140,208,100); Spin(Loops,LoopsSpin,220,208,draft.nLoops);
    Button(UiLanguage::Text(L"+产物"),AddOutput,480,299,88); Button(UiLanguage::Text(L"减一格"),RemoveOutput,576,299,88);
    if(draft.arrOutputs.size()>12) { Button(UiLanguage::Text(L"上一页"),PrevOutput,675,299,88); Button(UiLanguage::Text(L"下一页"),NextOutput,773,299,88); }
    Label(UiLanguage::Text(L"步骤（按顺序执行）"),22,253,390);
    auto pList=Control(_T("LISTBOX"),_T(""),StepList,22,287,375,202,WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP);
    for(size_t nIndex=0;nIndex<draft.arrSteps.size();++nIndex) { CString strText; strText.Format(_T("%d. %s"),(int)nIndex+1,UiLanguage::Text(GetInfo(draft.arrSteps[nIndex].eMethod).pName)); pList->SendMessage(LB_ADDSTRING,0,reinterpret_cast<LPARAM>(strText.GetString())); }
    pList->SendMessage(LB_SETCURSEL,m_nStepIndex);
    Button(UiLanguage::Text(L"添加"),StepAdd,22,497,80); Button(UiLanguage::Text(L"删除"),StepRemove,114,497,80); Button(UiLanguage::Text(L"上移"),StepUp,206,497,80); Button(UiLanguage::Text(L"下移"),StepDown,298,497,80);
    if(m_nStepIndex<0) return;
    auto& step=draft.arrSteps[m_nStepIndex];
    Label(UiLanguage::Text(L"当前步骤"),430,353,120);
    auto pCombo=Control(_T("COMBOBOX"),_T(""),StepType,550,345,305,185,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    for(int nIndex=0;nIndex<4;++nIndex) { pCombo->SendMessage(CB_ADDSTRING,0,reinterpret_cast<LPARAM>(UiLanguage::Text(GetInfo(StepMethods[nIndex]).pName))); if(step.eMethod==StepMethods[nIndex]) pCombo->SendMessage(CB_SETCURSEL,nIndex); }
    if(step.eMethod==Deploying || step.eMethod==Filling) {
        AddSlot(Secondary,0,430,395,65,step.eMethod==Deploying ? UiLanguage::Text(L"工具/材料") : UiLanguage::Text(L"流体"));
        if(step.eMethod==Deploying) Check(UiLanguage::Text(L"保留手持物品"),StepKeep,step.bKeepHeld,550,404,305);
    }
    if(step.eMethod==Cutting) {
        Check(UiLanguage::Text(L"自定义时间"),StepTimeCheck,step.bCustomTime,430,401,220);
        Edit(step.nTime,StepTime,680,401,150); Spin(StepTime,StepTimeSpin,810,401,step.nTime);
        GetDlgItem(StepTime)->EnableWindow(step.bCustomTime); GetDlgItem(StepTimeSpin)->EnableWindow(step.bCustomTime);
        Label(UiLanguage::Text(L"tick（默认 100）"),680,439,205);
    }
    Label(UiLanguage::Text(L"各步骤自动使用过渡物品作为输入和输出。"),430,490,455);
}
void CDlgCreate::AddEntry(bool bOutput,RecipeEntryKind eKind) {
    auto& draft=Draft(); auto& arrEntries=bOutput ? draft.arrOutputs : draft.arrInputs; const auto& info=GetInfo(m_eMethod);
    int nTotal=0; for(const auto& entry:arrEntries) if(entry.eKind==eKind) ++nTotal;
    int nLimit=eKind==RecipeFluid ? (bOutput ? info.nFluidsOut : info.nFluidsIn) : (bOutput ? info.nItemsOut : info.nItemsIn);
    if(m_eMethod==SequencedAssembly && bOutput) nLimit=256;
    if(nTotal>=nLimit) { AfxMessageBox(UiLanguage::Text(L"此类型格子已达到该方法的上限。")); return; }
    Entry entry; entry.eKind=eKind; if(m_eMethod==SequencedAssembly) entry.dChance=1;
    arrEntries.push_back(entry); (bOutput ? m_nOutputPage : m_nInputPage)=((int)arrEntries.size()-1)/12;
}
BOOL CDlgCreate::OnCommand(WPARAM wParam,LPARAM lParam) {
    UINT uId=LOWORD(wParam),uNotification=HIWORD(wParam); if(m_bRebuilding) return CDialogEx::OnCommand(wParam,lParam);
    if(uId==TargetVersion && uNotification==CBN_SELCHANGE) {
        SaveControls();
        m_eVersion = m_comboVersions.GetCurSel()==0 ? Version::Minecraft1201 : Version::Minecraft1211;
        if(!m_bCustomLoops) m_arrDrafts[SequencedAssembly].nLoops = DefaultLoops(m_eVersion);
        Rebuild(); return TRUE;
    }
    if(uId==IDC_COMBOCREATE && uNotification==CBN_SELCHANGE) {
        SaveControls(); m_eMethod=static_cast<Method>(m_comboMethods.GetItemData(m_comboMethods.GetCurSel())); m_nInputPage=m_nOutputPage=0; m_nStepIndex=-1; Rebuild(); return TRUE;
    }
    if(uId>=AddInput && uId<=StepTimeSpin && (uNotification==BN_CLICKED || uNotification==CBN_SELCHANGE || uNotification==LBN_SELCHANGE)) {
        SaveControls(); auto& draft=Draft();
        switch(uId) {
        case AddInput: AddEntry(false,RecipeItem); break; case AddInputFluid: AddEntry(false,RecipeFluid); break;
        case AddOutput: AddEntry(true,RecipeItem); break; case AddOutputFluid: AddEntry(true,RecipeFluid); break;
        case RemoveInput: if(draft.arrInputs.size()>1) draft.arrInputs.pop_back(); break;
        case RemoveOutput: if(draft.arrOutputs.size()>1) draft.arrOutputs.pop_back(); break;
        case PrevInput: --m_nInputPage; break; case NextInput: ++m_nInputPage; break;
        case PrevOutput: --m_nOutputPage; break; case NextOutput: ++m_nOutputPage; break;
        case GridApply: { BOOL bRowsValid,bColumnsValid; UINT uRows=GetDlgItemInt(Rows,&bRowsValid,FALSE),uColumns=GetDlgItemInt(Columns,&bColumnsValid,FALSE);
            if(!bRowsValid || !bColumnsValid || uRows<1 || uRows>9 || uColumns<1 || uColumns>9) { AfxMessageBox(UiLanguage::Text(L"行数和列数须为 1–9。")); return TRUE; }
            draft.nRows=(int)uRows; draft.nColumns=(int)uColumns; break; }
        case StepList: m_nStepIndex=(int)GetDlgItem(StepList)->SendMessage(LB_GETCURSEL); break;
        case StepType: if(m_nStepIndex>=0) { int nSelectedIndex=(int)GetDlgItem(StepType)->SendMessage(CB_GETCURSEL); if(nSelectedIndex>=0 && nSelectedIndex<4) {
            draft.arrSteps[m_nStepIndex].eMethod=StepMethods[nSelectedIndex]; draft.arrSteps[m_nStepIndex].secondary=Entry();
            draft.arrSteps[m_nStepIndex].secondary.eKind=nSelectedIndex==3 ? RecipeFluid : RecipeItem;
        } } break;
        case StepAdd: draft.arrSteps.push_back(Step()); m_nStepIndex=(int)draft.arrSteps.size()-1; break;
        case StepRemove: if(m_nStepIndex>=0) { draft.arrSteps.erase(draft.arrSteps.begin()+m_nStepIndex); m_nStepIndex=(std::min)(m_nStepIndex,(int)draft.arrSteps.size()-1); } break;
        case StepUp: if(m_nStepIndex>0) { std::swap(draft.arrSteps[m_nStepIndex],draft.arrSteps[m_nStepIndex-1]); --m_nStepIndex; } break;
        case StepDown: if(m_nStepIndex>=0 && m_nStepIndex+1<(int)draft.arrSteps.size()) { std::swap(draft.arrSteps[m_nStepIndex],draft.arrSteps[m_nStepIndex+1]); ++m_nStepIndex; } break;
        default: break;
        }
        m_nInputPage=max(0,min(m_nInputPage,((int)draft.arrInputs.size()-1)/12));
        m_nOutputPage=max(0,min(m_nOutputPage,((int)draft.arrOutputs.size()-1)/12)); Rebuild(); return TRUE;
    }
    return CDialogEx::OnCommand(wParam,lParam);
}
int CDlgCreate::Hit(CPoint ptPoint) const { for(size_t nIndex=0;nIndex<m_arrSlots.size();++nIndex) if(m_arrSlots[nIndex].rcRect.PtInRect(ptPoint)) return (int)nIndex; return -1; }
void CDlgCreate::Select(const Slot& slot) {
    const bool bAllowTags = slot.eGroup != Output && slot.eGroup != Transition;
    if (!bAllowTags && (m_pItemSource == nullptr || m_pItemSource->GetSize() == 0)) {
        AfxMessageBox(UiLanguage::Text(L"还没有可选的物品，请先在主界面点击“导入 jar 文件”。"), MB_ICONINFORMATION);
        return;
    }
    SaveControls(); Entry& entry=EntryAt(slot);
    CDlgSelect select(this); select.SetItemList(m_pItemSource); select.SetCatalog(m_pCatalog,entry.eKind);
    select.SetAllowTags(bAllowTags);
    bool bOutput=slot.eGroup==Output;
    int nCount=bOutput ? 64 : 1;
    if(slot.eGroup==Input && (m_eMethod==Mixing || m_eMethod==Compacting)) {
        nCount=64; for(int nIndex=0;nIndex<(int)Draft().arrInputs.size();++nIndex) if(nIndex!=slot.nIndex && Draft().arrInputs[nIndex].eKind==RecipeItem && !Draft().arrInputs[nIndex].strId.empty()) nCount-=Draft().arrInputs[nIndex].nCount;
        if(nCount<1 && entry.eKind==RecipeItem) { AfxMessageBox(UiLanguage::Text(L"物品原料总数已经达到 64。")); return; }
    }
    select.SetCountLimit(nCount); select.SetChanceMode(bOutput && m_eMethod!=MechanicalCrafting,m_eMethod==SequencedAssembly);
    select.SetInitialSelection(CString(entry.strId.c_str()),entry.nCount); select.SetInitialDetails(entry.eKind,entry.dChance);
    if(select.DoModal()==IDOK) { entry.strId=select.GetSelectedItem().GetString(); entry.nCount=select.GetSelectedCount(); entry.eKind=select.GetSelectedKind(); entry.dChance=select.GetSelectedChance(); Rebuild(); }
}
void CDlgCreate::OnPaint() {
    CPaintDC dc(this); auto pOldFont=dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
    for(size_t nIndex=0;nIndex<m_arrSlots.size();++nIndex) {
        const auto& slot=m_arrSlots[nIndex]; auto& entry=EntryAt(slot); CRect rcSlot=slot.rcRect;
        dc.FillSolidRect(rcSlot,(int)nIndex==m_nHoverIndex ? RGB(176,176,176) : RGB(139,139,139)); dc.Draw3dRect(rcSlot,RGB(55,55,55),RGB(255,255,255));
        if(!entry.strId.empty()) {
            if(RecipeIsTag(entry.strId)) { dc.SetTextColor(RGB(255,255,255)); dc.DrawText(L"#",rcSlot,DT_CENTER | DT_VCENTER | DT_SINGLELINE); }
            else if(!m_pTextures || !m_pTextures->Draw(&dc,rcSlot,CString(entry.strId.c_str()))) { CRect rcText=rcSlot; rcText.DeflateRect(3,3); dc.SetTextColor(RGB(30,30,30)); dc.DrawText(CString(entry.strId.c_str()),rcText,DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS); }
            CString strValue;
            if(entry.eKind==RecipeFluid) strValue.Format(_T("%d mB"),entry.nCount); else if(entry.nCount>1) strValue.Format(_T("%d"),entry.nCount);
            CRect rcText=rcSlot; rcText.top=rcText.bottom-(int)(22*m_dScale); rcText.DeflateRect(2,0); dc.SetTextColor(RGB(255,255,255)); dc.DrawText(strValue,rcText,DT_RIGHT | DT_SINGLELINE);
            if(slot.eGroup==Output && entry.eKind==RecipeItem && (m_eMethod==SequencedAssembly || entry.dChance!=100)) {
                strValue.Format(m_eMethod==SequencedAssembly ? _T("w:%.4g") : _T("%.4g%%"),entry.dChance); CRect rcText=rcSlot; rcText.bottom=rcText.top+(int)(22*m_dScale); dc.SetTextColor(RGB(255,255,255)); dc.DrawText(strValue,rcText,DT_LEFT | DT_SINGLELINE);
            }
        }
        CRect rcLabel=rcSlot; rcLabel.top=rcSlot.bottom+2; rcLabel.bottom=rcLabel.top+(int)(27*m_dScale); dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
        dc.DrawText(slot.strLabel,rcLabel,DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    dc.SelectObject(pOldFont);
}
void CDlgCreate::OnLButtonDown(UINT uFlags,CPoint ptPoint) { int nSelectedIndex=Hit(ptPoint); if(nSelectedIndex>=0) { Slot slot=m_arrSlots[nSelectedIndex]; Select(slot); } CDialogEx::OnLButtonDown(uFlags,ptPoint); }
void CDlgCreate::OnRButtonDown(UINT uFlags,CPoint ptPoint) { int nSelectedIndex=Hit(ptPoint); if(nSelectedIndex>=0) { auto& entry=EntryAt(m_arrSlots[nSelectedIndex]); entry.strId.clear(); entry.nCount=1; entry.dChance=m_eMethod==SequencedAssembly ? 1 : 100; SaveControls(); Rebuild(); } CDialogEx::OnRButtonDown(uFlags,ptPoint); }
void CDlgCreate::OnMouseMove(UINT uFlags,CPoint ptPoint) {
    int nSelectedIndex=Hit(ptPoint); if(nSelectedIndex!=m_nHoverIndex) { m_nHoverIndex=nSelectedIndex; Invalidate(FALSE); }
    if(!m_bTracking) { TRACKMOUSEEVENT tme={sizeof(TRACKMOUSEEVENT),TME_LEAVE,m_hWnd,0}; m_bTracking=TrackMouseEvent(&tme)!=FALSE; }
    CDialogEx::OnMouseMove(uFlags,ptPoint);
}
void CDlgCreate::OnMouseLeave() { m_bTracking=false; m_nHoverIndex=-1; Invalidate(FALSE); CDialogEx::OnMouseLeave(); }
BOOL CDlgCreate::PreTranslateMessage(MSG* pMsg) { m_toolTips.RelayEvent(pMsg); return CDialogEx::PreTranslateMessage(pMsg); }
void CDlgCreate::OnOK() {
    SaveControls();
    if(m_eMethod==MechanicalCrafting) { // Apply typed dimensions even when the grid button was not pressed.
        BOOL bRowsValid,bColumnsValid; UINT uRows=GetDlgItemInt(Rows,&bRowsValid,FALSE),uColumns=GetDlgItemInt(Columns,&bColumnsValid,FALSE);
        if(!bRowsValid || !bColumnsValid || uRows<1 || uRows>9 || uColumns<1 || uColumns>9) { AfxMessageBox(UiLanguage::Text(L"行数和列数须为 1–9。")); return; }
        Draft().nRows=(int)uRows; Draft().nColumns=(int)uColumns;
    }
    std::wstring strScript,strError;
    if(!Build(m_eMethod,Draft(),strScript,strError,m_eVersion)) { AfxMessageBox(UiLanguage::Text(strError.c_str()),MB_ICONINFORMATION); return; }
    m_strScript=strScript.c_str(); CDialogEx::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgCreate,CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
