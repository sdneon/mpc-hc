#pragma once
#include <afxwin.h>
class CMPCThemeGroupBox :  public CStatic
{
    DECLARE_DYNAMIC(CMPCThemeGroupBox)
public:
    CMPCThemeGroupBox();
    virtual ~CMPCThemeGroupBox();
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg void OnEnable(BOOL bEnable);
    //CStatic does not implement WM_SETFONT. Needed for manually created GroupBox
    afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetFont(WPARAM wParam, LPARAM lParam);
protected:
    HFONT manuallySetFont;
};

