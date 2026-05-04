#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"

class CButtonInfo
{
public:
    CString m_strText;
    UINT m_nID;
    CButton m_btn;

    CButtonInfo();
    CButtonInfo(LPCTSTR pszText, UINT nID);
};

class CHorizontalNavBar
{
public:
    enum { BTN_SIZE = 60, BTN_SPACING = 10, MARGIN = 10 };

protected:
    std::vector<CButtonInfo> m_buttons;
    HWND m_hWndParent;

public:
    CHorizontalNavBar();
    virtual ~CHorizontalNavBar() {}

    int GetWidth() const;
    int GetHeight() const;
    void Create(HWND hWndParent);
    void Layout(CRect rcParent);
    void Show(BOOL bShow = TRUE);
};

class CVerticalNavBar : 
    public CWindowImpl<CVerticalNavBar, CWindow>,
    public INavigationBar
{
public:
    enum { BTN_SIZE = 60, BTN_SPACING = 10, MARGIN = 10 };

    DECLARE_WND_CLASS(NULL)

    BEGIN_MSG_MAP(CVerticalNavBar)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    END_MSG_MAP()

protected:
    std::vector<CButtonInfo> m_buttons;
    HWND m_hWndParent;

public:
    CVerticalNavBar();
    virtual ~CVerticalNavBar() {}

    virtual HWND GetHwnd() const { return m_hWnd; }
    virtual int GetWidth() const;
    virtual int GetHeight() const;

    virtual HWND Create(HWND hWndParent, const RECT& rc);
    virtual void Show(BOOL bShow = TRUE);
    virtual void MoveWindow(const RECT& rc);

protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void CreateButtons();
    void LayoutButtons();
};
