#pragma once
#include "afxdialogex.h"
#include "FarmersDelightRecipe.h"
#include "CreateAddonRecipe.h"
#include "RecipeDialogUI.h"
#include <memory>

class CDlgOther : public CDialogEx {
    DECLARE_DYNAMIC(CDlgOther)
public:
    CDlgOther(CWnd* parent = nullptr);
    ~CDlgOther();
    void SetItemSource(const CStringArray* source) { m_source=source; }
    void SetCatalog(const RecipeTypeMap* catalog) { m_catalog=catalog; }
    void SetTextureStore(CItemTextureStore* textures) { m_textures=textures; }
    CString GetRecipeScript() const { return m_script; }
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLGOTHER };
#endif
protected:
    void DoDataExchange(CDataExchange* dx) override;
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wp, LPARAM lp) override;
    BOOL PreTranslateMessage(MSG* message) override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnRButtonDown(UINT flags, CPoint point);
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnExperienceSpin(NMHDR* header, LRESULT* result);
    DECLARE_MESSAGE_MAP()
private:
    enum class Group { Input, Output, Container, Tool, FluidInput, FluidOutput };
    struct Slot { Group group; int index; CRect rect; CString label; };
    const CStringArray* m_source=nullptr;
    const RecipeTypeMap* m_catalog=nullptr;
    CItemTextureStore* m_textures=nullptr;
    FarmersDelightRecipe::Draft m_drafts[2];
    CreateAddonRecipe::Draft m_addonDrafts[CreateAddonRecipe::MethodCount];
    int m_mod=0, m_addonMethods[2]={0,4}, m_inputPage=0, m_outputPage=0;
    CreateAddonRecipe::Method AddonMethod() const { return static_cast<CreateAddonRecipe::Method>(m_addonMethods[m_mod-1]); }
    CreateAddonRecipe::Draft& AddonDraft() { return m_addonDrafts[AddonMethod()]; }
    void PopulateMethods();
    void RebuildAddon();
    void ChangeAddonSlots(Group group, bool add);
    FarmersDelightRecipe::Method m_method=FarmersDelightRecipe::Method::Cooking;
    FarmersDelightRecipe::Version m_version=FarmersDelightRecipe::Version::Minecraft1211;
    CComboBox m_mods, m_methods, m_versions;
    CRecipeDialogUI m_ui;
    std::vector<std::unique_ptr<CWnd>> m_controls;
    std::vector<Slot> m_slots;
    CString m_script;
    int m_hover=-1, m_soundMode=0;
    bool m_rebuilding=false, m_tracking=false;
    FarmersDelightRecipe::Draft& Draft() { return m_drafts[m_method==FarmersDelightRecipe::Method::Cooking ? 0 : 1]; }
    CWnd* Control(LPCTSTR cls,const CString& text,UINT id,int x,int y,int w,int h,DWORD style=0);
    void Label(const CString& text,int x,int y,int w);
    void Edit(const CString& text,UINT id,int x,int y,int w,bool integer=false);
    void Spin(UINT edit,UINT id,int x,int y,int value);
    CWnd* Combo(UINT id,int x,int y,int w,const std::vector<CString>& choices,int selected);
    void SaveControls();
    void Rebuild();
    void AddSlot(Group group,int index,int x,int y,int size,const CString& label);
    void UpdateTips();
    FarmersDelightRecipe::Entry& EntryAt(const Slot& slot);
    int Hit(CPoint point) const;
    void Select(const Slot& slot);
};
