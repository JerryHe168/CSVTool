#include "stdafx.h"
#include "StatsListView.h"

HWND CStatsListView::Create(HWND hWndParent, RECT& rc)
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
    
    m_hWnd = CWindowImpl<CStatsListView, CListViewCtrl>::Create(
        hWndParent, &rc, NULL, dwStyle, WS_EX_CLIENTEDGE);
    
    return m_hWnd;
}

void CStatsListView::Show(BOOL bShow /* = TRUE */)
{
    if (m_hWnd)
        CWindowImpl<CStatsListView, CListViewCtrl>::ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}

void CStatsListView::MoveWindow(const RECT& rc)
{
    if (m_hWnd)
        CWindowImpl<CStatsListView, CListViewCtrl>::MoveWindow(&rc, TRUE);
}

void CStatsListView::Refresh()
{
    if (!m_hWnd) return;

    DeleteAllItems();

    int nColCount = GetHeader().GetItemCount();
    for (int i = 0; i < nColCount; ++i)
    {
        DeleteColumn(0);
    }

    InsertColumn(0, _T("统计项"), LVCFMT_LEFT, 180);
    InsertColumn(1, _T("统计值"), LVCFMT_LEFT, 180);

    if (!m_pModel || !m_pModel->IsLoaded())
    {
        InsertItem(0, _T("请先打开CSV文件"));
        SetItemText(0, 1, _T("-"));
        return;
    }

    std::vector<std::pair<CString, CString>> stats;
    m_pModel->GetStatistics(stats);

    for (size_t i = 0; i < stats.size(); ++i)
    {
        InsertItem((int)i, stats[i].first);
        SetItemText((int)i, 1, stats[i].second);
    }

    AutoSizeColumns();
}

void CStatsListView::AutoSizeColumns()
{
    int nColCount = GetHeader().GetItemCount();
    for (int i = 0; i < nColCount; ++i)
    {
        SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

LRESULT CStatsListView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    return 0;
}
