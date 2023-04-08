/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2020 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "MainFrm.h"
#include "PlayerPreView.h"
#include "CMPCTheme.h"
#include "mplayerc.h"

#define PREVIEW_TOOLTIP_BOTTOM 1

// CPrevView

CPreView::CPreView(CMainFrame* pMainFrame)
    : m_pMainFrame(pMainFrame) {
}

BOOL CPreView::SetWindowTextW(LPCWSTR lpString) {
    m_tooltipstr = lpString;

    CRect rect;
    GetClientRect(&rect);

    if (PREVIEW_TOOLTIP_BOTTOM) {
        rect.top = rect.bottom - m_caption - m_border;
        rect.bottom = rect.bottom - m_border;
    } else {
        rect.top = m_border;
        rect.bottom = m_caption + m_border;
    }
    rect.left += 10;
    rect.right -= 10;

    InvalidateRect(rect);

    return ::SetWindowTextW(m_hWnd, lpString);
}

void CPreView::GetVideoRect(LPRECT lpRect) {
    m_view.GetClientRect(lpRect);
}

HWND CPreView::GetVideoHWND() {
    return m_view.GetSafeHwnd();
}

IMPLEMENT_DYNAMIC(CPreView, CWnd)

BEGIN_MESSAGE_MAP(CPreView, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CPreView message handlers

BOOL CPreView::PreCreateWindow(CREATESTRUCT& cs) {
    if (!CWnd::PreCreateWindow(cs)) {
        return FALSE;
    }

    cs.style &= ~WS_BORDER;

    return TRUE;
}

int CPreView::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    m_caption = m_pMainFrame->m_dpi.ScaleY(20);

    CRect rc;
    GetClientRect(&rc);

    if (AfxGetAppSettings().bMPCTheme) {
        m_border = 1;
    } else {
        m_border = 5;
    }

    m_videorect.left = m_border;
    m_videorect.right = rc.right - m_border;
    if (PREVIEW_TOOLTIP_BOTTOM) { //bottom tooltip
        m_videorect.top = m_border;
        m_videorect.bottom = rc.bottom - m_caption;
    } else {
        m_videorect.top = m_caption;
        m_videorect.bottom = rc.bottom - m_border;
    }

    if (!m_view.Create(nullptr, nullptr, WS_CHILD | WS_VISIBLE, m_videorect, this, 0)) {
        return -1;
    }

    ScaleFont();
    SetColor();

    return 0;
}

void CPreView::OnPaint() {
    CPaintDC dc(this);

    CRect rcBar;
    GetClientRect(&rcBar);

    CDC mdc;
    mdc.CreateCompatibleDC(&dc);

    CBitmap bm;
    bm.CreateCompatibleBitmap(&dc, rcBar.Width(), rcBar.Height());
    CBitmap* pOldBm = mdc.SelectObject(&bm);
    mdc.SetBkMode(TRANSPARENT);

    COLORREF bg = GetSysColor(COLOR_BTNFACE);
    COLORREF frameLight = RGB(255, 255, 255);
    COLORREF frameShadow = GetSysColor(COLOR_BTNSHADOW);

    if (AfxGetAppSettings().bMPCTheme) {
        bg = CMPCTheme::CMPCTheme::MenuBGColor;
        frameLight = frameShadow = CMPCTheme::NoBorderColor;
        m_crText = CMPCTheme::TextFGColor;
    } else {
        m_crText = 0;
    }

    mdc.FillSolidRect(0, 0, rcBar.Width(), rcBar.Height(), bg); //fill

    mdc.FillSolidRect(0, 0, rcBar.Width(), 1, frameLight); //top border
    mdc.FillSolidRect(0, rcBar.Height() - 1, rcBar.Width(), 1, frameShadow); //bottom border
    mdc.FillSolidRect(0, 0, 1, rcBar.Height() - 1, frameLight); //left border
    mdc.FillSolidRect(rcBar.right - 1, 0, 1, rcBar.Height(), frameShadow); //right border

    CRect rtime(rcBar);
    if (PREVIEW_TOOLTIP_BOTTOM) {
        rtime.top = rcBar.Height() - m_caption;
        rtime.bottom = rcBar.Height();
    } else {
        rtime.top = 0;
        rtime.bottom = m_caption;
    }

    // text
    mdc.SelectObject(&m_font);
    mdc.SetTextColor(m_crText);
    mdc.DrawTextW(m_tooltipstr, m_tooltipstr.GetLength(), &rtime, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    dc.ExcludeClipRect(m_videorect);
    dc.BitBlt(0, 0, rcBar.Width(), rcBar.Height(), &mdc, 0, 0, SRCCOPY);

    mdc.SelectObject(pOldBm);
    bm.DeleteObject();
    mdc.DeleteDC();
}

void CPreView::OnShowWindow(BOOL bShow, UINT nStatus) {
    if (bShow) {
        m_pMainFrame->SetPreviewVideoPosition();
    }

    __super::OnShowWindow(bShow, nStatus);
}

void CPreView::SetWindowSize() {
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(GetParent()->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &mi);

    CRect wr;
    GetParent()->GetClientRect(&wr);

    int w = (mi.rcWork.right - mi.rcWork.left) * m_relativeSize / 100;
    // the preview size should not be larger than half size of the main window, but not less than 160
    w = std::max(160, std::min(w, wr.Width() / 2));

    CSize vs;

    vs = m_pMainFrame->GetVideoSizeWithRotation(true);
    if (vs.cx == 0) {
        vs.cx = 160;
        vs.cy = 90;
    }

    int h = w * vs.cy / vs.cx;
    w += m_border * 2;
    h += m_caption + m_border;

    CRect rc;
    GetClientRect(&rc);
    if (rc.Width() != w || rc.Height() != h) {
        SetWindowPos(nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        GetClientRect(&rc);
        m_videorect.right = rc.right - m_border;
        if (PREVIEW_TOOLTIP_BOTTOM) { //bottom tooltip
            m_videorect.bottom = rc.bottom - m_caption;
        } else {
            m_videorect.bottom = rc.bottom - m_border;
        }

        m_view.SetWindowPos(nullptr, 0, 0, m_videorect.Width(), m_videorect.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void CPreView::ScaleFont() {
    m_font.DeleteObject();
    CMPCThemeUtil::getFontByType(m_font, nullptr, nullptr, CMPCThemeUtil::MessageFont);
}

void CPreView::SetColor() {
    const auto bUseDarkTheme = AfxGetAppSettings().bMPCTheme;
}
