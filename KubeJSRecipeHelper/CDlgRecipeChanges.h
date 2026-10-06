#pragma once
#include "afxdialogex.h"
#include "RecipeChanges.h"
#include "RecipeCatalog.h"
#include "RecipeDialogUI.h"
#include <memory>

class CDlgRecipeChanges : public CDialogEx {
    DECLARE_DYNAMIC(CDlgRecipeChanges)
public:
    CDlgRecipeChanges(bool remove,CWnd* parent=nullptr);
    void SetItemSource(const CStringArray* source) { m_source=source; }
    void SetCatalog(const RecipeTypeMap* catalog) { m_catalog=catalog; }
    void SetTextureStore(CItemTextureStore* textures) { m_textures=textures; }
    CString GetRecipeScript() const { return m_script; }
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM wp,LPARAM lp) override;
    BOOL PreTranslateMessage(MSG* message) override;
    void OnOK() override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT flags,CPoint point);
    afx_msg void OnRButtonDown(UINT flags,CPoint point);
    afx_msg void OnMouseMove(UINT flags,CPoint point);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    struct Slot { bool replacement; CRect rect; CString label; };
    bool m_remove, m_rebuilding=false, m_tracking=false;
    int m_hover=-1;
    RecipeChanges::Method m_method;
    RecipeChanges::Draft m_drafts[6];
    const CStringArray* m_source=nullptr;
    const RecipeTypeMap* m_catalog=nullptr;
    CItemTextureStore* m_textures=nullptr;
    CRecipeDialogUI m_ui;
    std::vector<std::unique_ptr<CWnd>> m_controls;
    std::vector<Slot> m_slots;
    std::vector<UINT> m_radios;
    CString m_script;
    RecipeChanges::Draft& Draft() { return m_drafts[(int)m_method]; }
    std::wstring& EntryAt(const Slot& slot) { return slot.replacement ? Draft().replacement : Draft().item; }
    CWnd* Control(LPCTSTR cls,const CString& text,UINT id,int x,int y,int w,int h,DWORD style=0);
    void Label(const CString& text,int x,int y,int w);
    void SaveControls();
    void Rebuild();
    void UpdateTips();
    void AddSlot(bool replacement,int x,int y,const CString& label);
    int Hit(CPoint point) const;
    void Select(const Slot& slot);
};
