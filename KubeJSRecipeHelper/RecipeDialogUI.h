#pragma once

#include "afxdialogex.h"
#include "Language.h"
#include "ItemTextureStore.h"
#include <algorithm>
#include <vector>

// Shared spacing, monitor fitting and inventory rendering, based on CDlgCreate.
// Coordinates are logical pixels at 96 DPI; selection dialogs do not use this helper.
class CRecipeDialogUI
{
public:
    void Initialize(CDialogEx* pDialog, int nWidth, int nHeight)
    {
        m_pDialog = pDialog;
        CClientDC dc(pDialog);
        const double dDpiScale = dc.GetDeviceCaps(LOGPIXELSX) / 96.0;
        m_dScale = dDpiScale;
        MONITORINFO monitor = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(MonitorFromWindow(pDialog->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &monitor))
            m_dScale = (std::min)(m_dScale, (std::min)(
                (monitor.rcWork.right - monitor.rcWork.left - 50) / double(nWidth),
                (monitor.rcWork.bottom - monitor.rcWork.top - 65) / double(nHeight)));

        // Scale text along with controls when fitting a small working area.
        LOGFONT font = {};
        if (m_dScale < dDpiScale && pDialog->GetFont() && pDialog->GetFont()->GetLogFont(&font))
        {
            font.lfHeight = static_cast<LONG>(font.lfHeight * m_dScale / dDpiScale);
            if (m_font.CreateFontIndirect(&font))
            {
                pDialog->SetFont(&m_font);
                for (CWnd* pChild = pDialog->GetWindow(GW_CHILD); pChild; pChild = pChild->GetNextWindow())
                    pChild->SetFont(&m_font);
            }
        }
        CRect rcOuter = Rect(0, 0, nWidth, nHeight);
        AdjustWindowRectEx(&rcOuter, pDialog->GetStyle(), FALSE, pDialog->GetExStyle());
        pDialog->SetWindowPos(nullptr, 0, 0, rcOuter.Width(), rcOuter.Height(), SWP_NOMOVE | SWP_NOZORDER);
        pDialog->CenterWindow();
        if (pDialog->GetDlgItem(IDOK)) Move(IDOK, nWidth - 224, nHeight - 44, 100, 30);
        if (pDialog->GetDlgItem(IDCANCEL)) Move(IDCANCEL, nWidth - 110, nHeight - 44, 90, 30);
        m_toolTips.Create(pDialog, TTS_ALWAYSTIP);
        m_toolTips.SetMaxTipWidth(500);
        m_toolTips.Activate(TRUE);
    }

    CRect Rect(int nX, int nY, int nWidth, int nHeight) const
    {
        return CRect(static_cast<int>(nX * m_dScale), static_cast<int>(nY * m_dScale),
            static_cast<int>((nX + nWidth) * m_dScale), static_cast<int>((nY + nHeight) * m_dScale));
    }

    void Move(UINT uId, int nX, int nY, int nWidth, int nHeight)
    {
        if (CWnd* pControl = m_pDialog->GetDlgItem(uId)) pControl->MoveWindow(Rect(nX, nY, nWidth, nHeight));
    }

    void Relay(MSG* pMessage) { if (m_toolTips.GetSafeHwnd()) m_toolTips.RelayEvent(pMessage); }

    void ClearTips()
    {
        if (m_toolTips.GetSafeHwnd())
            for (auto uId : m_arrTipIds) m_toolTips.DelTool(m_pDialog, uId);
        m_arrTipIds.clear();
    }

    void Tip(UINT_PTR uId, const CRect& rcRect, const CString& strLabel, const CString& strItem, int nCount)
    {
        CString strText = strLabel + UiLanguage::Text(L"：左键选择，右键清空");
        if (!strItem.IsEmpty()) strText.Format(UiLanguage::Text(L"%s\n%s\n数量：%d"), strLabel.GetString(), strItem.GetString(), nCount);
        if (std::find(m_arrTipIds.begin(), m_arrTipIds.end(), uId) == m_arrTipIds.end())
        {
            m_toolTips.AddTool(m_pDialog, strText, rcRect, uId);
            m_arrTipIds.push_back(uId);
        }
        else m_toolTips.UpdateTipText(strText, m_pDialog, uId);
    }

    void Text(CDC* pDC, const CString& strText, CRect rcRect, bool bMuted = false,
        UINT uFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) const
    {
        const int nSavedDC = pDC->SaveDC();
        pDC->SelectObject(m_pDialog->GetFont());
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(GetSysColor(bMuted ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
        pDC->DrawText(strText, rcRect, uFormat);
        pDC->RestoreDC(nSavedDC);
    }

    void Slot(CDC* pDC, CRect rcRect, bool bHover, const CString& strItem, int nCount, CItemTextureStore* pTextures) const
    {
        const int nSavedDC = pDC->SaveDC();
        pDC->SelectObject(m_pDialog->GetFont());
        pDC->SetBkMode(TRANSPARENT);
        pDC->FillSolidRect(rcRect, bHover ? RGB(176, 176, 176) : RGB(139, 139, 139));
        pDC->Draw3dRect(rcRect, RGB(55, 55, 55), RGB(255, 255, 255));
        if (!strItem.IsEmpty())
        {
            if (strItem[0] == L'#')
            {
                pDC->SetTextColor(RGB(255,255,255));
                pDC->DrawText(L"#", rcRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            }
            else if (!pTextures || !pTextures->Draw(pDC, rcRect, strItem))
            {
                CRect rcText = rcRect;
                rcText.DeflateRect(3, 3);
                if (nCount > 1) rcText.bottom -= static_cast<int>(22 * m_dScale);
                pDC->SetTextColor(RGB(30, 30, 30));
                pDC->DrawText(strItem, rcText, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
            }
            if (nCount > 1)
            {
                CString strValue;
                strValue.Format(_T("%d"), nCount);
                CRect rcText = rcRect;
                rcText.top = rcText.bottom - static_cast<int>(22 * m_dScale);
                rcText.DeflateRect(3, 0);
                CRect rcShadow = rcText;
                rcShadow.OffsetRect(1, 1);
                pDC->SetTextColor(RGB(55, 55, 55));
                pDC->DrawText(strValue, rcShadow, DT_RIGHT | DT_SINGLELINE);
                pDC->SetTextColor(RGB(255, 255, 255));
                pDC->DrawText(strValue, rcText, DT_RIGHT | DT_SINGLELINE);
            }
        }
        pDC->RestoreDC(nSavedDC);
    }

    void Arrow(CDC* pDC, const CRect& rcRect) const
    {
        const int nSavedDC = pDC->SaveDC();
        const int nY = rcRect.CenterPoint().y, nHead = static_cast<int>(8 * m_dScale);
        CPen pen(PS_SOLID, (std::max)(1, static_cast<int>(2 * m_dScale)), GetSysColor(COLOR_GRAYTEXT));
        pDC->SelectObject(&pen);
        pDC->MoveTo(rcRect.left, nY); pDC->LineTo(rcRect.right, nY);
        pDC->MoveTo(rcRect.right - nHead, nY - nHead); pDC->LineTo(rcRect.right, nY);
        pDC->LineTo(rcRect.right - nHead, nY + nHead);
        pDC->RestoreDC(nSavedDC);
    }

private:
    CDialogEx* m_pDialog = nullptr;
    double m_dScale = 1;
    CFont m_font;
    CToolTipCtrl m_toolTips;
    std::vector<UINT_PTR> m_arrTipIds;
};
