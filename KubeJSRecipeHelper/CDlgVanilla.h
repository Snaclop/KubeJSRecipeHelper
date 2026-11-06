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
    CDlgVanilla(CWnd* pParent = nullptr);
    void SetItemSource(const CStringArray* pItemSource) { m_pItemSource = pItemSource; }
    void SetCatalog(const RecipeTypeMap* pCatalog) { m_pCatalog = pCatalog; }
    void SetTextureStore(CItemTextureStore* pTextures) { m_pTextures = pTextures; }
    CString GetRecipeScript() const { return m_strScript; }
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
    BOOL PreTranslateMessage(MSG* pMessage) override;
    void OnOK() override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT uFlags, CPoint ptPoint);
    afx_msg void OnRButtonDown(UINT uFlags, CPoint ptPoint);
    afx_msg void OnMouseMove(UINT uFlags, CPoint ptPoint);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    struct Slot { int nIndex; bool bOutput; CRect rcRect; CString strLabel; };
    VanillaRecipe::DraftBank m_arrDrafts;
    VanillaRecipe::Method m_eMethod = VanillaRecipe::Method::Shaped;
    CRecipeDialogUI m_ui;
    CComboBox m_comboMethods;
    std::vector<std::unique_ptr<CWnd>> m_arrFixedControls, m_arrControls;
    std::vector<Slot> m_arrSlots;
    const CStringArray* m_pItemSource = nullptr;
    const RecipeTypeMap* m_pCatalog = nullptr;
    CItemTextureStore* m_pTextures = nullptr;
    CString m_strScript;
    int m_nHoverIndex = -1;
    bool m_bTracking = false, m_bRebuilding = false;
    CRect m_rcArrow{0, 0, 0, 0};
    VanillaRecipe::Draft& Draft() { return m_arrDrafts.Get(m_eMethod); }
    VanillaRecipe::Entry& EntryAt(const Slot& slot);
    CWnd* Control(LPCTSTR pszClass, const CString& strText, UINT uId, int nX, int nY, int nWidth, int nHeight, DWORD dwStyle = 0, bool bFixed = false);
    void Label(const CString& strText, int nX, int nY, int nWidth);
    void Rebuild();
    void SaveControls();
    void AddSlot(int nIndex, bool bOutput, int nX, int nY, int nSize, const CString& strLabel);
    void UpdateTips();
    int Hit(CPoint ptPoint) const;
    void Select(const Slot& slot);
};
