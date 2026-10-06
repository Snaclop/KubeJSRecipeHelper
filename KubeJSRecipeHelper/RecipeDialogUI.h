#pragma once

#include "afxdialogex.h"
#include "ItemTextureStore.h"
#include <algorithm>
#include <vector>

// Shared spacing, monitor fitting and inventory rendering, based on CDlgCreate.
// Coordinates are logical pixels at 96 DPI; selection dialogs do not use this helper.
class CRecipeDialogUI
{
public:
    void Initialize(CDialogEx* dialog, int width, int height)
    {
        m_dialog = dialog;
        CClientDC dc(dialog);
        const double dpiScale = dc.GetDeviceCaps(LOGPIXELSX) / 96.0;
        m_scale = dpiScale;
        MONITORINFO monitor = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(MonitorFromWindow(dialog->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &monitor))
            m_scale = (std::min)(m_scale, (std::min)(
                (monitor.rcWork.right - monitor.rcWork.left - 50) / double(width),
                (monitor.rcWork.bottom - monitor.rcWork.top - 65) / double(height)));

        // Scale text along with controls when fitting a small working area.
        LOGFONT font = {};
        if (m_scale < dpiScale && dialog->GetFont() && dialog->GetFont()->GetLogFont(&font))
        {
            font.lfHeight = static_cast<LONG>(font.lfHeight * m_scale / dpiScale);
            if (m_font.CreateFontIndirect(&font))
            {
                dialog->SetFont(&m_font);
                for (CWnd* child = dialog->GetWindow(GW_CHILD); child; child = child->GetNextWindow())
                    child->SetFont(&m_font);
            }
        }
        CRect outer = Rect(0, 0, width, height);
        AdjustWindowRectEx(&outer, dialog->GetStyle(), FALSE, dialog->GetExStyle());
        dialog->SetWindowPos(nullptr, 0, 0, outer.Width(), outer.Height(), SWP_NOMOVE | SWP_NOZORDER);
        dialog->CenterWindow();
        if (dialog->GetDlgItem(IDOK)) Move(IDOK, width - 224, height - 44, 100, 30);
        if (dialog->GetDlgItem(IDCANCEL)) Move(IDCANCEL, width - 110, height - 44, 90, 30);
        m_tips.Create(dialog, TTS_ALWAYSTIP);
        m_tips.SetMaxTipWidth(500);
        m_tips.Activate(TRUE);
    }

    CRect Rect(int x, int y, int width, int height) const
    {
        return CRect(static_cast<int>(x * m_scale), static_cast<int>(y * m_scale),
            static_cast<int>((x + width) * m_scale), static_cast<int>((y + height) * m_scale));
    }

    void Move(UINT id, int x, int y, int width, int height)
    {
        if (CWnd* control = m_dialog->GetDlgItem(id)) control->MoveWindow(Rect(x, y, width, height));
    }

    void Relay(MSG* message) { if (m_tips.GetSafeHwnd()) m_tips.RelayEvent(message); }

    void ClearTips()
    {
        if (m_tips.GetSafeHwnd())
            for (auto id : m_tipIds) m_tips.DelTool(m_dialog, id);
        m_tipIds.clear();
    }

    void Tip(UINT_PTR id, const CRect& rect, const CString& label, const CString& item, int count)
    {
        CString text = label + _T("：左键选择，右键清空");
        if (!item.IsEmpty()) text.Format(_T("%s\n%s\n数量：%d"), label.GetString(), item.GetString(), count);
        if (std::find(m_tipIds.begin(), m_tipIds.end(), id) == m_tipIds.end())
        {
            m_tips.AddTool(m_dialog, text, rect, id);
            m_tipIds.push_back(id);
        }
        else m_tips.UpdateTipText(text, m_dialog, id);
    }

    void Text(CDC* dc, const CString& text, CRect rect, bool muted = false,
        UINT format = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) const
    {
        const int saved = dc->SaveDC();
        dc->SelectObject(m_dialog->GetFont());
        dc->SetBkMode(TRANSPARENT);
        dc->SetTextColor(GetSysColor(muted ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
        dc->DrawText(text, rect, format);
        dc->RestoreDC(saved);
    }

    void Slot(CDC* dc, CRect rect, bool hover, const CString& item, int count, CItemTextureStore* textures) const
    {
        const int saved = dc->SaveDC();
        dc->SelectObject(m_dialog->GetFont());
        dc->SetBkMode(TRANSPARENT);
        dc->FillSolidRect(rect, hover ? RGB(176, 176, 176) : RGB(139, 139, 139));
        dc->Draw3dRect(rect, RGB(55, 55, 55), RGB(255, 255, 255));
        if (!item.IsEmpty())
        {
            if (!textures || !textures->Draw(dc, rect, item))
            {
                CRect text = rect;
                text.DeflateRect(3, 3);
                if (count > 1) text.bottom -= static_cast<int>(22 * m_scale);
                dc->SetTextColor(RGB(30, 30, 30));
                dc->DrawText(item, text, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
            }
            if (count > 1)
            {
                CString value;
                value.Format(_T("%d"), count);
                CRect text = rect;
                text.top = text.bottom - static_cast<int>(22 * m_scale);
                text.DeflateRect(3, 0);
                CRect shadow = text;
                shadow.OffsetRect(1, 1);
                dc->SetTextColor(RGB(55, 55, 55));
                dc->DrawText(value, shadow, DT_RIGHT | DT_SINGLELINE);
                dc->SetTextColor(RGB(255, 255, 255));
                dc->DrawText(value, text, DT_RIGHT | DT_SINGLELINE);
            }
        }
        dc->RestoreDC(saved);
    }

    void Arrow(CDC* dc, const CRect& rect) const
    {
        const int saved = dc->SaveDC();
        const int y = rect.CenterPoint().y, head = static_cast<int>(8 * m_scale);
        CPen pen(PS_SOLID, (std::max)(1, static_cast<int>(2 * m_scale)), GetSysColor(COLOR_GRAYTEXT));
        dc->SelectObject(&pen);
        dc->MoveTo(rect.left, y); dc->LineTo(rect.right, y);
        dc->MoveTo(rect.right - head, y - head); dc->LineTo(rect.right, y);
        dc->LineTo(rect.right - head, y + head);
        dc->RestoreDC(saved);
    }

private:
    CDialogEx* m_dialog = nullptr;
    double m_scale = 1;
    CFont m_font;
    CToolTipCtrl m_tips;
    std::vector<UINT_PTR> m_tipIds;
};
