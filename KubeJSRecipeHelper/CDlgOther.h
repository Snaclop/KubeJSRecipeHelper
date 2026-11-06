#pragma once
#include "afxdialogex.h"
#include "ModRecipe.h"
#include "RecipeCatalog.h"
#include "RecipeDialogUI.h"
#include <memory>

class CDlgOther : public CDialogEx {
    DECLARE_DYNAMIC(CDlgOther)
public:
    CDlgOther(CWnd* pParent = nullptr);
    ~CDlgOther();
    void SetItemSource(const CStringArray* pItemSource) { m_pItemSource = pItemSource; }
    void SetCatalog(const RecipeTypeMap* pCatalog) { m_pCatalog = pCatalog; }
    void SetTextureStore(CItemTextureStore* pTextures) { m_pTextures = pTextures; }
    CString GetRecipeScript() const { return m_strScript; }
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLGOTHER };
#endif
protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
    BOOL PreTranslateMessage(MSG* pMessage) override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT uFlags, CPoint ptPoint);
    afx_msg void OnRButtonDown(UINT uFlags, CPoint ptPoint);
    afx_msg void OnMouseMove(UINT uFlags, CPoint ptPoint);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    struct Slot { size_t nGroupIndex, nIndex; CRect rcRect; CString strLabel; };
    const CStringArray* m_pItemSource = nullptr;
    const RecipeTypeMap* m_pCatalog = nullptr;
    CItemTextureStore* m_pTextures = nullptr;
    ModRecipe::Catalog m_definitions;
    std::vector<std::vector<ModRecipe::Draft>> m_arrDrafts;
    std::vector<int> m_arrRecipeIndices;
    int m_nModIndex = 0, m_nFieldPage = 0, m_nHoverIndex = -1;
    std::string m_strVersion, m_strLocale;
    std::map<std::string, int> m_mapGroupPages;
    std::vector<size_t> m_arrVisibleFields;
    CComboBox m_comboMods, m_comboMethods, m_comboVersions;
    CRecipeDialogUI m_ui;
    std::vector<std::unique_ptr<CWnd>> m_arrControls;
    std::vector<Slot> m_arrSlots;
    CString m_strScript;
    bool m_bRebuilding = false, m_bTracking = false;
    const ModRecipe::Recipe& Recipe() const;
    ModRecipe::Draft& Draft();
    ModRecipe::Entry& EntryAt(const Slot& slot);
    CString Label(const ModRecipe::Json& jsonLabel) const;
    CWnd* Control(LPCTSTR pszClass, const CString& strText, UINT uId, int nX, int nY, int nWidth, int nHeight, DWORD dwStyle = 0);
    CWnd* Combo(UINT uId, int nX, int nY, int nWidth, const std::vector<CString>& arrChoices, int nSelectedIndex);
    void PopulateMethods();
    void PopulateVersions();
    void SaveControls();
    void Rebuild();
    void BuildGroups(bool bOutput);
    void BuildFields();
    void UpdateTips();
    void Select(const Slot& slot);
    int Hit(CPoint ptPoint) const;
};
