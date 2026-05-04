#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"

class CDataListView : 
    public CWindowImpl<CDataListView, CListViewCtrl>,
    public CViewBase
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

    BEGIN_MSG_MAP(CDataListView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
    END_MSG_MAP()

public:
    CDataListView() {}
    virtual ~CDataListView() {}

    virtual ViewType GetViewType() const { return VIEW_DATA; }
    virtual HWND Create(HWND hWndParent, RECT& rc);
    virtual void Show(BOOL bShow = TRUE);
    virtual void MoveWindow(const RECT& rc);
    virtual HWND GetHwnd() const { return m_hWnd; }
    virtual void Refresh();
    void AutoSizeColumns();

protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
