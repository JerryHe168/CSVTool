#include "stdafx.h"
#include "NavigationBar.h"

CButtonInfo::CButtonInfo()
    : m_nID(0)
{
}

CButtonInfo::CButtonInfo(LPCTSTR pszText, UINT nID)
    : m_strText(pszText)
    , m_nID(nID)
{
}

CHorizontalNavBar::CHorizontalNavBar()
    : m_hWndParent(NULL)
{
    m_buttons.push_back(CButtonInfo(_T("打开"), 1000));
    m_buttons.push_back(CButtonInfo(_T("保存"), 1001));
    m_buttons.push_back(CButtonInfo(_T("统计"), 1002));
    m_buttons.push_back(CButtonInfo(_T("处理"), 1003));
    m_buttons.push_back(CButtonInfo(_T("导出"), 1004));
}

int CHorizontalNavBar::GetWidth() const
{
    return (int)m_buttons.size() * BTN_SIZE + 
           ((int)m_buttons.size() - 1) * BTN_SPACING + 
           2 * MARGIN;
}

int CHorizontalNavBar::GetHeight() const
{
    return BTN_SIZE + 2 * MARGIN;
}

void CHorizontalNavBar::Create(HWND hWndParent)
{
    m_hWndParent = hWndParent;
    
    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
        
        CRect rcBtn(0, 0, BTN_SIZE, BTN_SIZE);
        m_buttons[i].m_btn.Create(hWndParent, &rcBtn, 
            m_buttons[i].m_strText, dwStyle, 0, m_buttons[i].m_nID);
        
        HFONT hFont = (HFONT)::SendMessage(hWndParent, WM_GETFONT, 0, 0);
        if (hFont)
            m_buttons[i].m_btn.SetFont(hFont);
    }
}

void CHorizontalNavBar::Layout(CRect rcParent)
{
    int x = MARGIN;
    int y = MARGIN;

    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        if (m_buttons[i].m_btn.m_hWnd)
        {
            m_buttons[i].m_btn.MoveWindow(x, y, BTN_SIZE, BTN_SIZE);
        }
        x += BTN_SIZE + BTN_SPACING;
    }
}

void CHorizontalNavBar::Show(BOOL bShow /* = TRUE */)
{
    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        if (m_buttons[i].m_btn.m_hWnd)
            ::ShowWindow(m_buttons[i].m_btn.m_hWnd, bShow ? SW_SHOW : SW_HIDE);
    }
}

CVerticalNavBar::CVerticalNavBar()
    : m_hWndParent(NULL)
{
    m_buttons.push_back(CButtonInfo(_T("数据\n清洗"), 1010));
    m_buttons.push_back(CButtonInfo(_T("数据\n转换"), 1011));
    m_buttons.push_back(CButtonInfo(_T("排序\n数据"), 1012));
    m_buttons.push_back(CButtonInfo(_T("数据\n统计"), 1013));
    m_buttons.push_back(CButtonInfo(_T("数据\n筛选"), 1014));
}

int CVerticalNavBar::GetWidth() const
{
    return BTN_SIZE + 2 * MARGIN;
}

int CVerticalNavBar::GetHeight() const
{
    return (int)m_buttons.size() * BTN_SIZE + 
           ((int)m_buttons.size() - 1) * BTN_SPACING + 
           2 * MARGIN;
}

HWND CVerticalNavBar::Create(HWND hWndParent, RECT& rc)
{
    m_hWndParent = hWndParent;
    
    DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN;
    
    m_hWnd = CWindowImpl<CVerticalNavBar, CWindow>::Create(
        hWndParent, &rc, NULL, dwStyle, 0);
    
    return m_hWnd;
}

void CVerticalNavBar::Show(BOOL bShow /* = TRUE */)
{
    if (m_hWnd)
    {
        ::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        for (size_t i = 0; i < m_buttons.size(); ++i)
        {
            if (m_buttons[i].m_btn.m_hWnd)
                ::ShowWindow(m_buttons[i].m_btn.m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        }
    }
}

void CVerticalNavBar::MoveWindow(const RECT& rc)
{
    if (m_hWnd)
    {
        ::MoveWindow(m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        LayoutButtons();
    }
}

LRESULT CVerticalNavBar::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CreateButtons();
    return 0;
}

LRESULT CVerticalNavBar::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CDCHandle dc = (HDC)wParam;
    CRect rcClient;
    GetClientRect(&rcClient);
    
    HBRUSH hBrush = ::GetSysColorBrush(COLOR_3DFACE);
    dc.FillRect(&rcClient, hBrush);
    
    return 1;
}

void CVerticalNavBar::CreateButtons()
{
    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE | WS_TABSTOP;
        
        CRect rcBtn(0, 0, BTN_SIZE, BTN_SIZE);
        m_buttons[i].m_btn.Create(m_hWnd, &rcBtn, 
            m_buttons[i].m_strText, dwStyle, 0, m_buttons[i].m_nID);
        
        HFONT hFont = GetParent().GetFont();
        if (hFont)
            m_buttons[i].m_btn.SetFont(hFont);
    }
    
    LayoutButtons();
}

void CVerticalNavBar::LayoutButtons()
{
    CRect rcClient;
    GetClientRect(&rcClient);

    int y = MARGIN;
    int x = MARGIN;

    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        if (m_buttons[i].m_btn.m_hWnd)
        {
            m_buttons[i].m_btn.MoveWindow(x, y, BTN_SIZE, BTN_SIZE);
        }
        y += BTN_SIZE + BTN_SPACING;
    }
}
