#pragma once
#include "afxdialogex.h"
#include "RecipeChanges.h"
#include "RecipeCatalog.h"
#include "RecipeDialogUI.h"
#include <memory>

class CDlgRecipeChanges : public CDialogEx {
    DECLARE_DYNAMIC(CDlgRecipeChanges)
public:
    CDlgRecipeChanges(bool bRemove,CWnd* pParent=nullptr);
    void SetItemSource(const CStringArray* pItemSource) { m_pItemSource=pItemSource; }
    void SetCatalog(const RecipeTypeMap* pCatalog) { m_pCatalog=pCatalog; }
    void SetTextureStore(CItemTextureStore* pTextures) { m_pTextures=pTextures; }
    CString GetRecipeScript() const { return m_strScript; }
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM wParam,LPARAM lParam) override;
    BOOL PreTranslateMessage(MSG* pMessage) override;
    void OnOK() override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT uFlags,CPoint ptPoint);
    afx_msg void OnRButtonDown(UINT uFlags,CPoint ptPoint);
    afx_msg void OnMouseMove(UINT uFlags,CPoint ptPoint);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    struct Slot { bool bReplacement; CRect rcRect; CString strLabel; };
    bool m_bRemove, m_bRebuilding=false, m_bTracking=false;
    int m_nHoverIndex=-1;
    RecipeChanges::Method m_eMethod;
    RecipeChanges::Draft m_arrDrafts[6];
    const CStringArray* m_pItemSource=nullptr;
    const RecipeTypeMap* m_pCatalog=nullptr;
    CItemTextureStore* m_pTextures=nullptr;
    CRecipeDialogUI m_ui;
    std::vector<std::unique_ptr<CWnd>> m_arrControls;
    std::vector<Slot> m_arrSlots;
    std::vector<UINT> m_arrRadioIds;
    CString m_strScript;
    RecipeChanges::Draft& Draft() { return m_arrDrafts[(int)m_eMethod]; }
    std::wstring& EntryAt(const Slot& slot) { return slot.bReplacement ? Draft().strReplacement : Draft().strItem; }
    CWnd* Control(LPCTSTR pszClass,const CString& strText,UINT uId,int nX,int nY,int nWidth,int nHeight,DWORD dwStyle=0);
    void Label(const CString& strText,int nX,int nY,int nWidth);
    void SaveControls();
    void Rebuild();
    void UpdateTips();
    void AddSlot(bool bReplacement,int nX,int nY,const CString& strLabel);
    int Hit(CPoint ptPoint) const;
    void Select(const Slot& slot);
};
