#include "stdafx.h"
#include "DataListView.h"

HWND CDataListView::Create(HWND hWndParent, RECT& rc)
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
    
    m_hWnd = CWindowImpl<CDataListView, CListViewCtrl>::Create(
        hWndParent, &rc, NULL, dwStyle, WS_EX_CLIENTEDGE);
    
    return m_hWnd;
}

void CDataListView::Show(BOOL bShow /* = TRUE */)
{
    if (m_hWnd)
        CWindowImpl<CDataListView, CListViewCtrl>::ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}

void CDataListView::MoveWindow(const RECT& rc)
{
    if (m_hWnd)
        CWindowImpl<CDataListView, CListViewCtrl>::MoveWindow(&rc, TRUE);
}

void CDataListView::Refresh()
{
    if (!m_hWnd) return;

    DeleteAllItems();

    while (DeleteColumn(0))
    {
    }

    if (!m_pModel)
    {
        return;
    }

    if (!m_pModel->IsLoaded())
    {
        return;
    }

    size_t nColCnt = m_pModel->GetColumnCount();
    if (nColCnt == 0)
    {
        return;
    }

    for (size_t i = 0; i < nColCnt; ++i)
    {
        CString strHeader = m_pModel->GetHeader(i);
        InsertColumn((int)i, strHeader, LVCFMT_LEFT, 120);
    }

    size_t nRowCnt = m_pModel->GetRowCount();
    size_t nMaxRows = 10000;
    
    for (size_t row = 0; row < nRowCnt && row < nMaxRows; ++row)
    {
        CString strFirst = m_pModel->GetCell(row, 0);
        InsertItem((int)row, strFirst);
        
        for (size_t col = 1; col < nColCnt; ++col)
        {
            CString strCell = m_pModel->GetCell(row, col);
            SetItemText((int)row, (int)col, strCell);
        }
    }

    AutoSizeColumns();
}

void CDataListView::AutoSizeColumns()
{
    if (!m_hWnd) return;
    
    HWND hHeader = ListView_GetHeader(m_hWnd);
    if (!hHeader) return;
    
    int nColCount = (int)::SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < nColCount; ++i)
    {
        SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

LRESULT CDataListView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    return 0;
}
