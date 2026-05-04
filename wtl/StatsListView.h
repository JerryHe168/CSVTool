#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"

class CStatsListView : 
    public CWindowImpl<CStatsListView, CListViewCtrl>,
    public CViewBase
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

    BEGIN_MSG_MAP(CStatsListView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
    END_MSG_MAP()

public:
    CStatsListView();
    virtual ~CStatsListView() {}

    virtual ViewType GetViewType() const { return VIEW_STATS; }
    virtual HWND Create(HWND hWndParent, const RECT& rc);
    virtual void Refresh();
    void AutoSizeColumns();

protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
