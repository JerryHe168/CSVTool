#include "stdafx.h"
#include "ViewInterfaces.h"

CViewBase::CViewBase()
    : m_hWnd(NULL)
    , m_pModel(NULL)
{
}

void CViewBase::Show(BOOL bShow /* = TRUE */)
{
    if (m_hWnd)
        ::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void CViewBase::MoveWindow(const RECT& rc)
{
    if (m_hWnd)
        ::MoveWindow(m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}
