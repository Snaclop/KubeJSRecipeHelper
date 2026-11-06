#pragma once
#include "afxdialogex.h"
#include "CreateRecipeModel.h"
#include "ItemTextureStore.h"
#include <memory>

class CDlgCreate : public CDialogEx {
    DECLARE_DYNAMIC(CDlgCreate)
public:
    CDlgCreate(CWnd* pParent = nullptr);
    ~CDlgCreate();
    void SetItemSource(const CStringArray* pItemSource) { m_pItemSource = pItemSource; }
    void SetCatalog(const RecipeTypeMap* pCatalog) { m_pCatalog = pCatalog; }
    void SetTextureStore(CItemTextureStore* pTextures) { m_pTextures = pTextures; }
    CString GetRecipeScript() const { return m_strScript; }
protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT uFlags, CPoint ptPoint);
    afx_msg void OnRButtonDown(UINT uFlags, CPoint ptPoint);
    afx_msg void OnMouseMove(UINT uFlags, CPoint ptPoint);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()
private:
    enum Group { Input, Output, Grid, Transition, Secondary };
    struct Slot { CRect rcRect; Group eGroup; int nIndex; CString strLabel; };
    const CStringArray* m_pItemSource = nullptr;
    const RecipeTypeMap* m_pCatalog = nullptr;
    CItemTextureStore* m_pTextures = nullptr;
    CreateRecipe::Draft m_arrDrafts[CreateRecipe::MethodCount];
    CreateRecipe::Method m_eMethod = CreateRecipe::Compacting;
    CComboBox m_comboMethods;
    CComboBox m_comboVersions;
    CreateRecipe::Version m_eVersion = CreateRecipe::Version::Minecraft1211;
    bool m_bCustomLoops = false;
    CToolTipCtrl m_toolTips;
    std::vector<std::unique_ptr<CWnd>> m_arrControls;
    std::vector<Slot> m_arrSlots;
    CString m_strScript;
    double m_dScale = 1;
    int m_nInputPage = 0, m_nOutputPage = 0, m_nStepIndex = -1, m_nHoverIndex = -1;
    bool m_bRebuilding = false, m_bTracking = false;
    CreateRecipe::Draft& Draft() { return m_arrDrafts[m_eMethod]; }
    CRect Rect(int nX, int nY, int nWidth, int nHeight) const;
    CWnd* Control(LPCTSTR pszClass, const CString& strText, UINT uId, int nX, int nY, int nWidth, int nHeight, DWORD dwStyle = 0);
    void Label(const CString& strText, int nX, int nY, int nWidth = 350);
    void Button(const CString& strText, UINT uId, int nX, int nY, int nWidth = 95);
    void Edit(int nValue, UINT uId, int nX, int nY, int nWidth = 100);
    void Check(const CString& strText, UINT uId, bool bChecked, int nX, int nY, int nWidth = 350);
    void Spin(UINT uEditId, UINT uId, int nX, int nY, int nValue, int nMaximum = 99999999);
    void Rebuild();
    void SaveControls();
    void AddSlot(Group eGroup, int nIndex, int nX, int nY, int nSize, const CString& strLabel);
    void PageSlots(Group eGroup, std::vector<CreateRecipe::Entry>& arrEntries, int nPage, int nX, int nY);
    CreateRecipe::Entry& EntryAt(const Slot& slot);
    int Hit(CPoint ptPoint) const;
    void Select(const Slot& slot);
    void AddEntry(bool bOutput, RecipeEntryKind eKind);
    void BuildSequence();
};
