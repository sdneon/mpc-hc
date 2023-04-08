#pragma once

class CMPCThemeButton : public CMFCButton
{
protected:
    void drawButton(HDC hdc, CRect rect, UINT state);
    CFont font;
    bool drawShield;
public:
    CMPCThemeButton();
    virtual ~CMPCThemeButton();
    void PreSubclassWindow();
    BOOL PreCreateWindow(CREATESTRUCT& cs);
    LRESULT setShieldIcon(WPARAM wParam, LPARAM lParam);
    static void drawButtonBase(CDC* pDC, CRect rect, CString strText, bool selected, bool highLighted, bool focused, bool disabled, bool thin, bool shield = false);
    DECLARE_DYNAMIC(CMPCThemeButton)
    DECLARE_MESSAGE_MAP()
    afx_msg void OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFont(CFont* pFont, BOOL bRedraw);
    afx_msg HFONT OnGetFont();
};

