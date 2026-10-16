#pragma once
#include "afxdialogex.h"
#include "ModRecipe.h"
#include "RecipeCatalog.h"
#include "RecipeDialogUI.h"
#include <memory>

class CDlgOther : public CDialogEx {
    DECLARE_DYNAMIC(CDlgOther)
public:
    CDlgOther(CWnd* parent = nullptr);
    ~CDlgOther();
    void SetItemSource(const CStringArray* source) { m_source = source; }
    void SetCatalog(const RecipeTypeMap* catalog) { m_catalog = catalog; }
    void SetTextureStore(CItemTextureStore* textures) { m_textures = textures; }
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
    DECLARE_MESSAGE_MAP()
private:
    struct Slot { size_t group, index; CRect rect; CString label; };
    const CStringArray* m_source = nullptr;
    const RecipeTypeMap* m_catalog = nullptr;
    CItemTextureStore* m_textures = nullptr;
    ModRecipe::Catalog m_definitions;
    std::vector<std::vector<ModRecipe::Draft>> m_drafts;
    std::vector<int> m_recipes;
    int m_mod = 0, m_fieldPage = 0, m_hover = -1;
    std::string m_version, m_locale;
    std::map<std::string, int> m_groupPages;
    std::vector<size_t> m_visibleFields;
    CComboBox m_mods, m_methods, m_versions;
    CRecipeDialogUI m_ui;
    std::vector<std::unique_ptr<CWnd>> m_controls;
    std::vector<Slot> m_slots;
    CString m_script;
    bool m_rebuilding = false, m_tracking = false;
    const ModRecipe::Recipe& Recipe() const;
    ModRecipe::Draft& Draft();
    ModRecipe::Entry& EntryAt(const Slot& slot);
    CString Label(const ModRecipe::Json& text) const;
    CWnd* Control(LPCTSTR cls, const CString& text, UINT id, int x, int y, int w, int h, DWORD style = 0);
    CWnd* Combo(UINT id, int x, int y, int w, const std::vector<CString>& choices, int selected);
    void PopulateMethods();
    void PopulateVersions();
    void SaveControls();
    void Rebuild();
    void BuildGroups(bool output);
    void BuildFields();
    void UpdateTips();
    void Select(const Slot& slot);
    int Hit(CPoint point) const;
};
