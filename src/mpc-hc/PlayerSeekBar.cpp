/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "PlayerSeekBar.h"
#include "MainFrm.h"
#include "mplayerc.h"
#include "CMPCTheme.h"


#define TOOLTIP_SHOW_DELAY 100
#define TOOLTIP_HIDE_TIMEOUT 3000
#define HOVER_CAPTURED_TIMEOUT 100
#define ADD_TO_BOTTOM_WITHOUT_CONTROLBAR 2
#define SEEK_DRAGGER_OVERLAP 5

IMPLEMENT_DYNAMIC(CPlayerSeekBar, CDialogBar)

CPlayerSeekBar::CPlayerSeekBar(CMainFrame* pMainFrame)
    : m_pMainFrame(pMainFrame)
    , m_rtStart(0)
    , m_rtStop(0)
    , m_rtPos(0)
    , m_bEnabled(false)
    , m_bHasDuration(false)
    , m_rtHoverPos(0)
    , m_cursor(AfxGetApp()->LoadStandardCursor(IDC_HAND))
    , m_bDraggingThumb(false)
    , m_bHoverThumb(false)
    , m_tooltipState(TOOLTIP_HIDDEN)
    , m_bIgnoreLastTooltipPoint(true)
    , m_lastDragSeekTickCount(0)
{
    ZeroMemory(&m_ti, sizeof(m_ti));
    m_ti.cbSize = sizeof(m_ti);

    GetEventd().Connect(m_eventc, {
        MpcEvent::DPI_CHANGED,
    }, std::bind(&CPlayerSeekBar::EventCallback, this, std::placeholders::_1));
}

CPlayerSeekBar::~CPlayerSeekBar()
{
}

BOOL CPlayerSeekBar::Create(CWnd* pParentWnd)
{
    if (!__super::Create(pParentWnd,
                         IDD_PLAYERSEEKBAR, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_BOTTOM, IDD_PLAYERSEEKBAR)) {
        return FALSE;
    }

    if (!AppIsThemeLoaded()) {
        CDC* pDC = GetWindowDC();
        if (CMPCThemeUtil::getFontByType(mpcThemeFont, pDC, GetParent(), CMPCThemeUtil::MessageFont)) {
            SetFont(&mpcThemeFont);
        }
        ReleaseDC(pDC);
    }

    // Should never be RTLed
    ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);


    m_tooltip.Create(this, TTS_NOPREFIX | TTS_ALWAYSTIP);
    m_tooltip.SetMaxTipWidth(-1);

    m_ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    m_ti.hwnd = m_hWnd;
    m_ti.uId = (UINT_PTR)m_hWnd;
    m_ti.hinst = AfxGetInstanceHandle();
    m_ti.lpszText = nullptr;

    m_tooltip.SendMessage(TTM_ADDTOOL, 0, (LPARAM)&m_ti);

    return TRUE;
}

void CPlayerSeekBar::EventCallback(MpcEvent ev)
{
    switch (ev) {
        case MpcEvent::DPI_CHANGED:
            m_pEnabledThumb = nullptr;
            m_pDisabledThumb = nullptr;
            break;

        default:
            ASSERT(FALSE);
    }
}

BOOL CPlayerSeekBar::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!__super::PreCreateWindow(cs)) {
        return FALSE;
    }

    m_dwStyle &= ~CBRS_BORDER_TOP;
    m_dwStyle &= ~CBRS_BORDER_BOTTOM;
    m_dwStyle |= CBRS_SIZE_FIXED;

    return TRUE;
}

CSize CPlayerSeekBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
    CSize ret = __super::CalcFixedLayout(bStretch, bHorz);
    const CAppSettings& s = AfxGetAppSettings();
    if (s.bMPCTheme && s.bModernSeekbar) {
        ret.cy = m_pMainFrame->m_dpi.ScaleY(5 + s.iModernSeekbarHeight); //expand the toolbar if using "fill" mode
    } else {
        ret.cy = m_pMainFrame->m_dpi.ScaleY(20);
    }
    if (!m_pMainFrame->m_controls.ControlChecked(CMainFrameControls::Toolbar::CONTROLS)) {
        ret.cy += ADD_TO_BOTTOM_WITHOUT_CONTROLBAR;
    }
    return ret;
}

void CPlayerSeekBar::MoveThumb(const CPoint& point)
{
    if (m_bHasDuration) {
        REFERENCE_TIME rtPos = PositionFromClientPoint(point);
        const CAppSettings& s = AfxGetAppSettings();
        REFERENCE_TIME duration = m_rtStop - m_rtStart;
        if (duration >= 600000000LL && s.bFastSeek && (GetKeyState(VK_SHIFT) >= 0)) {
            REFERENCE_TIME rtMaxDiff = s.bAllowInaccurateFastseek ? 200000000LL : std::min(100000000LL, duration / 30);
            rtPos = m_pMainFrame->GetClosestKeyFrame(rtPos, rtMaxDiff, rtMaxDiff);
        }
        SyncThumbToVideo(rtPos);
    }
}

void CPlayerSeekBar::SyncVideoToThumb()
{
    GetParent()->PostMessage(WM_HSCROLL, NULL, reinterpret_cast<LPARAM>(m_hWnd));
}

void CPlayerSeekBar::CheckScrollDistance(CPoint point, REFERENCE_TIME minimum_duration_change)
{
    ULONGLONG tickcount = GetTickCount64();
    ULONGLONG ticks_since_last_seek = tickcount - m_lastDragSeekTickCount;
    REFERENCE_TIME posdiff = m_rtHoverPos > m_rtPos ? m_rtHoverPos - m_rtPos : m_rtPos - m_rtHoverPos;

    if (minimum_duration_change == 0 || (ticks_since_last_seek >= 150ULL) && (posdiff >= minimum_duration_change) || (ticks_since_last_seek >= 1000ULL) && (posdiff >= minimum_duration_change / 2)) {
        m_lastDragSeekTickCount = tickcount;
        m_rtHoverPos = m_rtPos;
        m_hoverPoint = point;
        SyncVideoToThumb();
    }
}

long CPlayerSeekBar::ChannelPointFromPosition(REFERENCE_TIME rtPos) const
{
    rtPos = std::min(m_rtStop, std::max(m_rtStart, rtPos));
    long ret = 0;
    auto w = GetChannelRect().Width();
    if (m_bHasDuration) {
        ret = (long)(w * (rtPos - m_rtStart) / (m_rtStop - m_rtStart));
    }
    if (ret >= w) {
        ret = w - 1;
    }
    return ret;
}

REFERENCE_TIME CPlayerSeekBar::PositionFromClientPoint(const CPoint& point) const
{
    REFERENCE_TIME rtRet = -1;
    if (m_bHasDuration) {
        ASSERT(m_rtStart < m_rtStop);
        const CRect channelRect(GetChannelRect());
        auto channelPointX = (point.x < channelRect.left) ? channelRect.left :
                             (point.x > channelRect.right) ? channelRect.right : point.x;
        ASSERT(channelPointX >= channelRect.left && channelPointX <= channelRect.right);
        rtRet = m_rtStart + (channelPointX - channelRect.left) * (m_rtStop - m_rtStart) / channelRect.Width();
    }
    return rtRet;
}

void CPlayerSeekBar::SyncThumbToVideo(REFERENCE_TIME rtPos)
{
    m_rtPos = rtPos;
    if (m_bHasDuration) {
        CRect newThumbRect(GetThumbRect());
        bool bSetTaskbar = (rtPos <= 0);
        if (newThumbRect != m_lastThumbRect) {
            bSetTaskbar = true;
            InvalidateRect(newThumbRect | m_lastThumbRect);
        }
        if (bSetTaskbar && AfxGetAppSettings().bUseEnhancedTaskBar && m_pMainFrame->m_pTaskbarList) {
            VERIFY(S_OK == m_pMainFrame->m_pTaskbarList->SetProgressValue(m_pMainFrame->m_hWnd, std::max(m_rtPos, 1ll), m_rtStop));
        }
    }
}

void CPlayerSeekBar::CreateThumb(bool bEnabled, CDC& parentDC)
{
    auto& pThumb = bEnabled ? m_pEnabledThumb : m_pDisabledThumb;
    pThumb = std::unique_ptr<CDC>(new CDC());

    if (pThumb->CreateCompatibleDC(&parentDC)) {
        COLORREF
        white  = GetSysColor(COLOR_WINDOW),
        shadow = GetSysColor(COLOR_3DSHADOW),
        light  = GetSysColor(COLOR_3DHILIGHT),
        bkg    = GetSysColor(COLOR_BTNFACE);

        CRect r(GetThumbRect());
        r.MoveToXY(0, 0);
        CRect ri(GetInnerThumbRect(bEnabled, r));

        CBitmap bmp;
        VERIFY(bmp.CreateCompatibleBitmap(&parentDC, r.Width(), r.Height()));
        VERIFY(pThumb->SelectObject(bmp));

        if (AppIsThemeLoaded()) {
            //just a rectangle, we will draw from scratch
        } else {
            pThumb->Draw3dRect(&r, light, 0);
            r.DeflateRect(0, 0, 1, 1);
            pThumb->Draw3dRect(&r, light, shadow);
            r.DeflateRect(1, 1, 1, 1);

            if (bEnabled) {
                pThumb->ExcludeClipRect(ri);
                ri.InflateRect(0, 1, 0, 1);
                pThumb->FillSolidRect(ri, white);
                pThumb->SetPixel(ri.CenterPoint().x, ri.top, 0);
                pThumb->SetPixel(ri.CenterPoint().x, ri.bottom - 1, 0);
            }
            pThumb->ExcludeClipRect(ri);

            ri.InflateRect(1, 1, 1, 1);
            pThumb->Draw3dRect(&ri, shadow, bkg);
            pThumb->ExcludeClipRect(ri);

            CBrush b(bkg);
            pThumb->FillRect(&r, &b);
        }
    } else {
        ASSERT(FALSE);
    }
}

CRect CPlayerSeekBar::GetChannelRect() const
{
    CRect r;
    GetClientRect(&r);
    r.top += 1;
    if (m_pMainFrame->m_controls.ControlChecked(CMainFrameControls::Toolbar::CONTROLS)) {
        r.bottom += ADD_TO_BOTTOM_WITHOUT_CONTROLBAR;
    }

    const CAppSettings& s = AfxGetAppSettings();
    if (s.bMPCTheme && s.bModernSeekbar) { //no thumb so we can use all the space
        r.DeflateRect(m_pMainFrame->m_dpi.ScaleFloorX(2), m_pMainFrame->m_dpi.ScaleFloorX(2));
    } else {
        CSize sz(m_pMainFrame->m_dpi.ScaleFloorX(8), m_pMainFrame->m_dpi.ScaleFloorY(7) + 1);
        r.DeflateRect(sz.cx, sz.cy, sz.cx, sz.cy);
    }

    return r;
}

CRect CPlayerSeekBar::GetThumbRect() const
{
    const CRect channelRect(GetChannelRect());
    const long x = channelRect.left + ChannelPointFromPosition(m_rtPos);
    CSize s;
    s.cy = m_pMainFrame->m_dpi.ScaleFloorY(SEEK_DRAGGER_OVERLAP);
    s.cx = m_pMainFrame->m_dpi.TransposeScaledY(channelRect.Height()) / 2 + s.cy;
    CRect r(x + 1 - s.cx, channelRect.top - s.cy, x + s.cx, channelRect.bottom + s.cy);
    return r;
}

CRect CPlayerSeekBar::GetInnerThumbRect(bool bEnabled, const CRect& thumbRect) const
{
    CSize s(m_pMainFrame->m_dpi.ScaleFloorX(4) - 1, m_pMainFrame->m_dpi.ScaleFloorY(5));
    if (!bEnabled) {
        s.cy -= 1;
    }
    CRect r(thumbRect);
    r.DeflateRect(s.cx, s.cy, s.cx, s.cy);
    return r;
}

void CPlayerSeekBar::UpdateTooltip(const CPoint& point)
{
    CRect clientRect;
    GetClientRect(&clientRect);

    if (!m_bHasDuration || !clientRect.PtInRect(point) || DraggingThumb()) {
        HideToolTip();
        m_pMainFrame->PreviewWindowHide();
        return;
    }

    switch (m_tooltipState) {
        case TOOLTIP_HIDDEN: {
            // If mouse moved or the tooltip wasn't hidden by timeout
            if (point != m_tooltipPoint || m_bIgnoreLastTooltipPoint) {
                m_bIgnoreLastTooltipPoint = false;
                // Start show tooltip countdown
                m_tooltipState = TOOLTIP_TRIGGERED;
                VERIFY(SetTimer(TIMER_SHOWHIDE_TOOLTIP, TOOLTIP_SHOW_DELAY, nullptr));
                // Track mouse leave
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hWnd };
                VERIFY(TrackMouseEvent(&tme));
            }
        }
        break;
        case TOOLTIP_TRIGGERED:
            // Do nothing until tooltip is shown
            break;
        case TOOLTIP_VISIBLE:
            // Update the tooltip if needed
            ASSERT(!m_bIgnoreLastTooltipPoint);
            if (point != m_tooltipPoint) {
                m_tooltipPoint = point;
                if (!m_pMainFrame->CanPreviewUse()) {
                    UpdateToolTipText();
                    UpdateToolTipPosition(point);
                    VERIFY(SetTimer(TIMER_SHOWHIDE_TOOLTIP, TOOLTIP_HIDE_TIMEOUT, nullptr));
                }
            }
            break;
        default:
            ASSERT(FALSE);
    }
}

void CPlayerSeekBar::UpdateToolTipPosition(CPoint point)
{
    if (m_pMainFrame->CanPreviewUse()) {
        if (!m_pMainFrame->m_wndPreView.IsWindowVisible()) {
            m_pMainFrame->m_wndPreView.SetWindowSize();
        }

        CRect rc;
        m_pMainFrame->m_wndPreView.GetWindowRect(&rc);
        const int r_width = rc.Width();
        const int r_height = rc.Height();

        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

        point.x -= r_width / 2 - 2;
        point.y = GetChannelRect().TopLeft().y - (r_height + 13);
        ClientToScreen(&point);
        point.x = std::max(mi.rcWork.left + 5, std::min(point.x, mi.rcWork.right - r_width - 5));

        if (point.y <= 5) {
            const CRect r = mi.rcWork;
            if (!r.PtInRect(point)) {
                CPoint p(point);
                p.y = GetChannelRect().TopLeft().y + 30;
                ClientToScreen(&p);

                point.y = p.y;
            }
        }

        m_pMainFrame->m_wndPreView.SetWindowPos(&wndTopMost, point.x, point.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        CSize bubbleSize(m_tooltip.GetBubbleSize(&m_ti));
        CRect windowRect;
        GetWindowRect(windowRect);

        if (AfxGetAppSettings().nTimeTooltipPosition == TIME_TOOLTIP_ABOVE_SEEKBAR) {
            point.x -= bubbleSize.cx / 2 - 2;
            point.y = GetChannelRect().TopLeft().y - (bubbleSize.cy + m_pMainFrame->m_dpi.ScaleY(13));
        } else {
            point.x += m_pMainFrame->m_dpi.ScaleX(10);
            point.y += m_pMainFrame->m_dpi.ScaleY(20);
        }
        point.x = std::max(0l, std::min(point.x, windowRect.Width() - bubbleSize.cx));
        ClientToScreen(&point);

        m_tooltip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(point.x, point.y));
    }
}

void CPlayerSeekBar::GenerateToolTipText(REFERENCE_TIME rtPos)
{
    ASSERT(m_bHasDuration);

    CString time;
    GUID timeFormat = m_pMainFrame->GetTimeFormat();
    if (timeFormat == TIME_FORMAT_MEDIA_TIME) {
        DVD_HMSF_TIMECODE tcNow = RT2HMS_r(rtPos);
        if (tcNow.bHours > 0) {
            time.Format(_T("%02u:%02u:%02u"), tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
        } else {
            time.Format(_T("%02u:%02u"), tcNow.bMinutes, tcNow.bSeconds);
        }
    } else if (timeFormat == TIME_FORMAT_FRAME) {
        time.Format(_T("%I64d"), rtPos);
    } else {
        ASSERT(FALSE);
    }

    CComBSTR chapterName;
    {
        CAutoLock lock(&m_csChapterBag);
        if (m_pChapterBag) {
            REFERENCE_TIME rt = rtPos;
            m_pChapterBag->ChapLookup(&rt, &chapterName);
        }
    }

    if (chapterName.Length() == 0) {
        m_tooltipText = time;
    } else {
        m_tooltipText.Format(_T("%s - %s"), time.GetString(), static_cast<LPCTSTR>(chapterName));
    }
}

void CPlayerSeekBar::UpdateToolTipText()
{
    ASSERT(m_bHasDuration);
    REFERENCE_TIME rtPos = PositionFromClientPoint(m_tooltipPoint);
    GenerateToolTipText(rtPos);
    
    m_ti.lpszText = (LPTSTR)(LPCTSTR)m_tooltipText;
    m_tooltip.SetToolInfo(&m_ti);
}

void CPlayerSeekBar::UpdateToolTipTextPreview(REFERENCE_TIME rtPos)
{
    ASSERT(m_bHasDuration);
    GenerateToolTipText(rtPos);
    m_pMainFrame->m_wndPreView.SetWindowTextW(m_tooltipText);
}

void CPlayerSeekBar::Enable(bool bEnable)
{
    if (bEnable != m_bEnabled) {
        m_bEnabled = bEnable;
        Invalidate();
    }
}

void CPlayerSeekBar::HideToolTip()
{
    if (m_tooltipState != TOOLTIP_HIDDEN) {
        KillTimer(TIMER_SHOWHIDE_TOOLTIP);
        m_tooltip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
        m_tooltipState = TOOLTIP_HIDDEN;
    }
}

void CPlayerSeekBar::GetRange(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop) const
{
    rtStart = m_rtStart;
    rtStop = m_rtStop;
}

void CPlayerSeekBar::SetRange(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
    if (rtStart < rtStop) {
        if (m_rtStart != rtStart || m_rtStop != rtStop) {
            m_rtStart = rtStart;
            m_rtStop = rtStop;
            auto hasChapters = [&]() {
                CAutoLock lock(&m_csChapterBag);
                return m_pChapterBag && m_pChapterBag->ChapGetCount();
            };
            if (!m_bHasDuration || hasChapters()) {
                Invalidate();
            }
            m_bHasDuration = true;
        }
    } else {
        m_rtStart = 0;
        m_rtStop = 0;
        if (m_bHasDuration) {
            m_bHasDuration = false;
            HideToolTip();
            if (DraggingThumb()) {
                ReleaseCapture();
            }
            Invalidate();
        }
    }
}

REFERENCE_TIME CPlayerSeekBar::GetPos() const
{
    return m_rtPos;
}

void CPlayerSeekBar::SetPos(REFERENCE_TIME rtPos)
{
    if (DraggingThumb()) {
        return;
    }

    SyncThumbToVideo(rtPos);
}

bool CPlayerSeekBar::HasDuration() const
{
    return m_bHasDuration;
}

REFERENCE_TIME CPlayerSeekBar::GetDuration()
{
    return m_bHasDuration ? m_rtStop - m_rtStart : 0LL;
}

void CPlayerSeekBar::SetChapterBag(IDSMChapterBag* pCB)
{
    CAutoLock lock(&m_csChapterBag);
    m_pChapterBag = pCB;
    Invalidate();
}

void CPlayerSeekBar::RemoveChapters()
{
    SetChapterBag(nullptr);
}

bool CPlayerSeekBar::DraggingThumb()
{
    return m_bDraggingThumb;
}

BEGIN_MESSAGE_MAP(CPlayerSeekBar, CDialogBar)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONUP()
    ON_WM_XBUTTONDOWN()
    ON_WM_XBUTTONUP()
    ON_WM_XBUTTONDBLCLK()
    ON_WM_MBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_ERASEBKGND()
    ON_WM_SETCURSOR()
    ON_WM_TIMER()
    ON_WM_MOUSELEAVE()
    ON_WM_THEMECHANGED()
    ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

void CPlayerSeekBar::OnPaint()
{
    CPaintDC dc(this);

    COLORREF
    dark   = GetSysColor(COLOR_GRAYTEXT),
    white  = GetSysColor(COLOR_WINDOW),
    shadow = GetSysColor(COLOR_3DSHADOW),
    light  = GetSysColor(COLOR_3DHILIGHT),
    bkg    = GetSysColor(COLOR_BTNFACE);

    const CRect channelRect(GetChannelRect());
    auto funcMarkChannel = [&](REFERENCE_TIME pos, long verticalPadding, COLORREF markColor) {
        long markPos = channelRect.left + ChannelPointFromPosition(pos);
        CRect r(markPos, channelRect.top + verticalPadding, markPos + 1, channelRect.bottom - verticalPadding);
        if (r.right < channelRect.right) {
            r.right++;
        }
        ASSERT(r.right <= channelRect.right);
        dc.FillSolidRect(&r, markColor);
        dc.ExcludeClipRect(&r);
    };

    const CAppSettings& s = AfxGetAppSettings();
    if (s.bMPCTheme) {
        // Thumb
        if (!s.bModernSeekbar) { //no thumb while showing seek progress
            CRect r(GetThumbRect());
            if (DraggingThumb()) {
                dc.FillSolidRect(r, CMPCTheme::ScrollThumbDragColor);
            } else if (m_bHoverThumb) {
                dc.FillSolidRect(r, CMPCTheme::ScrollThumbHoverColor);
            } else if (m_bEnabled) {
                dc.FillSolidRect(r, CMPCTheme::ScrollThumbColor);
            } else {
                dc.FillSolidRect(r, CMPCTheme::ScrollBGColor);
            }
            CBrush fb;
            fb.CreateSolidBrush(CMPCTheme::NoBorderColor);
            dc.FrameRect(r, &fb);
            fb.DeleteObject();

            CRgn rg;
            VERIFY(rg.CreateRectRgnIndirect(&r));
            ExtSelectClipRgn(dc, rg, RGN_XOR);
            rg.DeleteObject();

            m_lastThumbRect = r;
        } else {
            CRect r(GetThumbRect());
            m_lastThumbRect = r;
        }

        if (m_bHasDuration) {
            // A-B Repeat
            REFERENCE_TIME aPos, bPos;
            bool aEnabled, bEnabled;
            if (m_pMainFrame->CheckABRepeat(aPos, bPos, aEnabled, bEnabled)) {
                if (aEnabled) {
                    funcMarkChannel(aPos, 1, CMPCTheme::SeekbarABColor);
                }
                if (bEnabled) {
                    funcMarkChannel(bPos, 1, CMPCTheme::SeekbarABColor);
                }
            }

            // Chapters
            CAutoLock lock(&m_csChapterBag);
            if (m_pChapterBag) {
                for (DWORD i = 0; i < m_pChapterBag->ChapGetCount(); i++) {
                    REFERENCE_TIME rtChap;
                    if (SUCCEEDED(m_pChapterBag->ChapGet(i, &rtChap, nullptr))) {
                        funcMarkChannel(rtChap, 1, CMPCTheme::SeekbarChapterColor);
                    } else {
                        ASSERT(FALSE);
                    }
                }
            }
        }

        // Channel
        {
            if (s.bModernSeekbar) {
                long seekPos = ChannelPointFromPosition(m_rtPos);
                CRect r, playedRect, unplayedRect, curPosRect;
                playedRect = channelRect;
                playedRect.right = playedRect.left + seekPos - m_pMainFrame->m_dpi.ScaleX(2) + 1;
                dc.FillSolidRect(&playedRect, CMPCTheme::ScrollProgressColor);
                unplayedRect = channelRect;
                unplayedRect.left = playedRect.left + seekPos + m_pMainFrame->m_dpi.ScaleX(2);
                dc.FillSolidRect(&unplayedRect, m_bEnabled ? CMPCTheme::ScrollBGColor : CMPCTheme::ScrollBGColor);
                curPosRect = channelRect;
                curPosRect.left = playedRect.right;
                curPosRect.right = unplayedRect.left;
                dc.FillSolidRect(&curPosRect, CMPCTheme::SeekbarCurrentPositionColor);
            } else {
                dc.FillSolidRect(&channelRect, m_bEnabled ? CMPCTheme::ScrollBGColor : CMPCTheme::ScrollBGColor);
            }
            CRect r(channelRect);
            CBrush fb;
            fb.CreateSolidBrush(CMPCTheme::NoBorderColor);
            dc.FrameRect(&r, &fb);
            fb.DeleteObject();
            dc.ExcludeClipRect(&r);
        }

        // Background
        {
            CRect r;
            GetClientRect(&r);
            dc.FillSolidRect(&r, CMPCTheme::ContentBGColor);
        }
    } else {
        // Thumb
        {
            auto& pThumb = m_bEnabled ? m_pEnabledThumb : m_pDisabledThumb;
            if (!pThumb) {
                CreateThumb(m_bEnabled, dc);
                ASSERT(pThumb);
            }
            CRect r(GetThumbRect());
            CRect ri(GetInnerThumbRect(m_bEnabled, r));

            CRgn rg, rgi;
            VERIFY(rg.CreateRectRgnIndirect(&r));
            VERIFY(rgi.CreateRectRgnIndirect(&ri));

            ExtSelectClipRgn(dc, rgi, RGN_DIFF);
            VERIFY(dc.BitBlt(r.TopLeft().x, r.TopLeft().y, r.Width(), r.Height(), pThumb.get(), 0, 0, SRCCOPY));
            ExtSelectClipRgn(dc, rg, RGN_XOR);

            m_lastThumbRect = r;
        }

        if (m_bHasDuration) {
            // A-B Repeat
            REFERENCE_TIME aPos, bPos;
            bool aEnabled, bEnabled;
            if (m_pMainFrame->CheckABRepeat(aPos, bPos, aEnabled, bEnabled)) {
                if (aEnabled) {
                    funcMarkChannel(aPos, 0, CMPCTheme::SeekbarABColor);
                }
                if (bEnabled) {
                    funcMarkChannel(bPos, 0, CMPCTheme::SeekbarABColor);
                }
            }

            // Chapters
            CAutoLock lock(&m_csChapterBag);
            if (m_pChapterBag) {
                for (DWORD i = 0; i < m_pChapterBag->ChapGetCount(); i++) {
                    REFERENCE_TIME rtChap;
                    if (SUCCEEDED(m_pChapterBag->ChapGet(i, &rtChap, nullptr))) {
                        funcMarkChannel(rtChap, 0, dark);
                    } else {
                        ASSERT(FALSE);
                    }
                }
            }
        }

        // Channel
        {
            dc.FillSolidRect(&channelRect, m_bEnabled ? white : bkg);
            CRect r(channelRect);
            r.InflateRect(1, 1);
            dc.Draw3dRect(&r, shadow, light);
            dc.ExcludeClipRect(&r);
        }

        // Background
        {
            CRect r;
            GetClientRect(&r);
            dc.FillSolidRect(&r, bkg);
        }
    }


}

void CPlayerSeekBar::OnLButtonDown(UINT nFlags, CPoint point)
{
    CRect clientRect;
    GetClientRect(&clientRect);
    if (m_bEnabled && m_bHasDuration && clientRect.PtInRect(point)) {
        SetCapture();
        m_bDraggingThumb = true;
        MoveThumb(point);
        CheckScrollDistance(point, 0);
        invalidateThumb();
    } else {
        if (!m_pMainFrame->m_fFullScreen) {
            MapWindowPoints(m_pMainFrame, &point, 1);
            m_pMainFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
        }
    }
}

void CPlayerSeekBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    OnLButtonDown(nFlags, point);
}

void CPlayerSeekBar::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (DraggingThumb()) {
        ReleaseCapture();
        // update video position if seekbar moved at least 250 ms or 1/100th of duration
        CheckScrollDistance(point, std::min(2500000LL, m_rtStop / 100));
        invalidateThumb();
    }
    checkHover(point);
}

void CPlayerSeekBar::OnXButtonDown(UINT nFlags, UINT nButton, CPoint point)
{
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    // do medium jumps when clicking mouse navigation buttons on the seekbar
    // if not dragging the seekbar thumb
    if (!DraggingThumb()) {
        switch (nButton) {
            case XBUTTON1:
                SendMessage(WM_COMMAND, ID_PLAY_SEEKBACKWARDMED);
                break;
            case XBUTTON2:
                SendMessage(WM_COMMAND, ID_PLAY_SEEKFORWARDMED);
                break;
        }
    }
}

void CPlayerSeekBar::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(nButton);
    UNREFERENCED_PARAMETER(point);
    // do nothing
}

void CPlayerSeekBar::OnXButtonDblClk(UINT nFlags, UINT nButton, CPoint point)
{
    OnXButtonDown(nFlags, nButton, point);
}

void CPlayerSeekBar::checkHover(CPoint point)
{
    CRect tRect(GetThumbRect());
    bool oldHover = m_bHoverThumb;
    m_bHoverThumb = false;
    if (m_bEnabled && m_bHasDuration && tRect.PtInRect(point)) {
        m_bHoverThumb = true;
    }

    if (m_bHoverThumb != oldHover) {
        invalidateThumb();
    }
}

void CPlayerSeekBar::invalidateThumb()
{
    CRect tRect(GetThumbRect());
    InvalidateRect(tRect);
}

void CPlayerSeekBar::OnMouseMove(UINT nFlags, CPoint point)
{

    if (DraggingThumb() && (nFlags & MK_LBUTTON)) {
        MoveThumb(point);
        // update video position if seekbar moved at least 500ms or 1/30th of duration
        CheckScrollDistance(point, std::min(5000000LL, m_rtStop / 30));
    }
    if (AfxGetAppSettings().fUseTimeTooltip) {
        UpdateTooltip(point);
    }

    if (m_bHasDuration && m_pMainFrame->CanPreviewUse()) {
        UpdateTooltip(point);

        checkHover(point);

        const OAFilterState fs = m_pMainFrame->GetMediaState();
        if (fs != -1) {
            if (m_pMainFrame->CanPreviewUse()) {
                UpdateToolTipPosition(point);
                PreviewWindowShow(point);
            }
        } else {
            m_pMainFrame->PreviewWindowHide();
        }
    }
}

BOOL CPlayerSeekBar::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

BOOL CPlayerSeekBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    BOOL ret = TRUE;
    if (m_bEnabled && m_bHasDuration) {
        ::SetCursor(m_cursor);
    } else {
        ret = __super::OnSetCursor(pWnd, nHitTest, message);
    }
    return ret;
}

void CPlayerSeekBar::OnTimer(UINT_PTR nIDEvent)
{
    CPoint point;
    VERIFY(GetCursorPos(&point));
    ScreenToClient(&point);
    switch (nIDEvent) {
        case TIMER_SHOWHIDE_TOOLTIP:
            if (m_tooltipState == TOOLTIP_TRIGGERED && m_bHasDuration) {
                m_tooltipPoint = point;
                UpdateToolTipText();
                if (!m_pMainFrame->CanPreviewUse()) {
                    m_tooltip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
                }
                UpdateToolTipPosition(point);
                m_tooltipState = TOOLTIP_VISIBLE;
                VERIFY(SetTimer(TIMER_SHOWHIDE_TOOLTIP, m_pMainFrame->CanPreviewUse() ? 10 : TOOLTIP_HIDE_TIMEOUT, nullptr));
            } else if (m_tooltipState == TOOLTIP_VISIBLE) {
                HideToolTip();
                PreviewWindowShow(point);
                ASSERT(!m_bIgnoreLastTooltipPoint);
                KillTimer(TIMER_SHOWHIDE_TOOLTIP);
            } else {
                KillTimer(TIMER_SHOWHIDE_TOOLTIP);
            }
            break;
        default:
            ASSERT(FALSE);
    }
}

void CPlayerSeekBar::OnMouseLeave()
{
    HideToolTip();
    m_pMainFrame->PreviewWindowHide();
    m_bIgnoreLastTooltipPoint = true;
    checkHover(CPoint(0, 0));
}

LRESULT CPlayerSeekBar::OnThemeChanged()
{
    m_pEnabledThumb = nullptr;
    m_pDisabledThumb = nullptr;
    return __super::OnThemeChanged();
}

void CPlayerSeekBar::OnCaptureChanged(CWnd* pWnd)
{
    ASSERT(m_bDraggingThumb);
    m_bDraggingThumb = false;
    if (!pWnd) {
        // HACK: windowed (not renderless) video renderers may not produce WM_MOUSEMOVE message here
        m_pMainFrame->UpdateControlState(CMainFrame::UPDATE_CHILDVIEW_CURSOR_HACK);
    }
}

void CPlayerSeekBar::PreviewWindowShow(CPoint point) {
    if (point.x != m_last_pointx_preview && !DraggingThumb()) {
        m_last_pointx_preview = point.x;
        if (m_pMainFrame->CanPreviewUse()) {
            REFERENCE_TIME newpos = std::clamp(PositionFromClientPoint(point), 0LL, m_rtStop);
            if (newpos != m_pos_preview) {
                if (GetKeyState(VK_SHIFT) >= 0) {
                    newpos = m_pMainFrame->GetClosestKeyFramePreview(newpos);
                }
            }
            if (newpos != m_pos_preview || !m_pMainFrame->m_wndPreView.IsWindowVisible()) {
                m_pos_preview = newpos;
                UpdateToolTipTextPreview(m_pos_preview);
                m_pMainFrame->PreviewWindowShow(m_pos_preview);
            }
        }
    }
}

//adipose: code came from mpc-be; seems to be a hidden way to
//disable seek preview permanently by middle clicking on the seekbar
//cannot be used to re-enable it, so perhaps a safety option if
//preview is misbehaving? leave in for now
void CPlayerSeekBar::OnMButtonDown(UINT nFlags, CPoint point) {
    if (m_pMainFrame->m_wndPreView.IsWindowVisible()) {
        m_pMainFrame->PreviewWindowHide();
        AfxGetAppSettings().fSeekPreview = !AfxGetAppSettings().fSeekPreview;
        OnMouseMove(nFlags, point);
    }
}
