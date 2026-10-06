#pragma once
#include "afxdialogex.h"
#include "RecipeDialogUI.h"
#include "RecipeCatalog.h"
#include "VanillaRecipe.h"
#include <memory>
#include <vector>

class CDlgVanilla : public CDialogEx {
    DECLARE_DYNAMIC(CDlgVanilla)
public:
    CDlgVanilla(CWnd* parent = nullptr);
    void SetItemSource(const CStringArray* source) { m_source = source; }
    void SetCatalog(const RecipeTypeMap* catalog) { m_catalog = catalog; }
    void SetTextureStore(CItemTextureStore* textures) { m_textures = textures; }
    CString GetRecipeScript() const { return m_script; }
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM wp, LPARAM lp) override;
    BOOL PreTranslateMessage(MSG* message) override;
    void OnOK() override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnRButtonDown(UINT flags, CPoint point);
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    struct Slot { int index; bool output; CRect rect; CString label; };
    VanillaRecipe::DraftBank m_drafts;
    VanillaRecipe::Method m_method = VanillaRecipe::Method::Shaped;
    CRecipeDialogUI m_ui;
    CComboBox m_methods;
    std::vector<std::unique_ptr<CWnd>> m_fixedControls, m_controls;
    std::vector<Slot> m_slots;
    const CStringArray* m_source = nullptr;
    const RecipeTypeMap* m_catalog = nullptr;
    CItemTextureStore* m_textures = nullptr;
    CString m_script;
    int m_hover = -1;
    bool m_tracking = false, m_rebuilding = false;
    CRect m_arrow{0, 0, 0, 0};
    VanillaRecipe::Draft& Draft() { return m_drafts.Get(m_method); }
    VanillaRecipe::Entry& EntryAt(const Slot& slot);
    CWnd* Control(LPCTSTR cls, const CString& text, UINT id, int x, int y, int w, int h, DWORD style = 0, bool fixed = false);
    void Label(const CString& text, int x, int y, int w);
    void Rebuild();
    void SaveControls();
    void AddSlot(int index, bool output, int x, int y, int size, const CString& label);
    void UpdateTips();
    int Hit(CPoint point) const;
    void Select(const Slot& slot);
};
