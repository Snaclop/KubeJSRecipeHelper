#pragma once
#include "afxdialogex.h"
#include "CreateRecipeModel.h"
#include "ItemTextureStore.h"
#include <memory>

class CDlgCreate : public CDialogEx {
    DECLARE_DYNAMIC(CDlgCreate)
public:
    CDlgCreate(CWnd* parent = nullptr);
    ~CDlgCreate();
    void SetItemSource(const CStringArray* source) { m_source = source; }
    void SetCatalog(const RecipeTypeMap* catalog) { m_catalog = catalog; }
    void SetTextureStore(CItemTextureStore* textures) { m_textures = textures; }
    CString GetRecipeScript() const { return m_script; }
protected:
    void DoDataExchange(CDataExchange* dx) override;
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wp, LPARAM lp) override;
    BOOL PreTranslateMessage(MSG* msg) override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnRButtonDown(UINT flags, CPoint point);
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    enum Group { Input, Output, Grid, Transition, Secondary };
    struct Slot { CRect rect; Group group; int index; CString label; };
    const CStringArray* m_source = nullptr;
    const RecipeTypeMap* m_catalog = nullptr;
    CItemTextureStore* m_textures = nullptr;
    CreateRecipe::Draft m_drafts[CreateRecipe::MethodCount];
    CreateRecipe::Method m_method = CreateRecipe::Compacting;
    CComboBox m_methods;
    CToolTipCtrl m_tips;
    std::vector<std::unique_ptr<CWnd>> m_controls;
    std::vector<Slot> m_slots;
    CString m_script;
    double m_scale = 1;
    int m_inputPage = 0, m_outputPage = 0, m_step = -1, m_hover = -1;
    bool m_rebuilding = false, m_tracking = false;
    CreateRecipe::Draft& Draft() { return m_drafts[m_method]; }
    CRect Rect(int x, int y, int w, int h) const;
    CWnd* Control(LPCTSTR cls, const CString& text, UINT id, int x, int y, int w, int h, DWORD style = 0);
    void Label(const CString& text, int x, int y, int w = 350);
    void Button(const CString& text, UINT id, int x, int y, int w = 95);
    void Edit(int value, UINT id, int x, int y, int w = 100);
    void Check(const CString& text, UINT id, bool checked, int x, int y, int w = 350);
    void Spin(UINT edit, UINT id, int x, int y, int value, int maximum = 99999999);
    void Rebuild();
    void SaveControls();
    void AddSlot(Group group, int index, int x, int y, int size, const CString& label);
    void PageSlots(Group group, std::vector<CreateRecipe::Entry>& entries, int page, int x, int y);
    CreateRecipe::Entry& EntryAt(const Slot& slot);
    int Hit(CPoint point) const;
    void Select(const Slot& slot);
    void AddEntry(bool output, RecipeEntryKind kind);
    void BuildSequence();
};
