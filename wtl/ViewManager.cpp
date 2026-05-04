#include "stdafx.h"
#include "ViewManager.h"
#include "DataListView.h"
#include "StatsListView.h"
#include "NavigationBar.h"

CViewManager::CViewManager()
    : m_pModel(NULL)
    , m_hWndParent(NULL)
    , m_currentView(VIEW_NONE)
{
}

void CViewManager::SetModel(IDataModel* pModel)
{
    m_pModel = pModel;
}

void CViewManager::SetParent(HWND hWndParent)
{
    m_hWndParent = hWndParent;
}

ViewType CViewManager::GetCurrentView() const
{
    return m_currentView;
}

void CViewManager::CreateViews(const RECT& rcDataArea)
{
    if (!m_hWndParent) return;

    m_pDataView.reset(new CDataListView());
    m_pDataView->SetModel(m_pModel);
    m_pDataView->Create(m_hWndParent, rcDataArea);
    m_pDataView->Show(FALSE);

    m_pStatsView.reset(new CStatsListView());
    m_pStatsView->SetModel(m_pModel);
    m_pStatsView->Create(m_hWndParent, rcDataArea);
    m_pStatsView->Show(FALSE);

    m_pVerticalNav.reset(new CVerticalNavBar());
    m_pVerticalNav->Create(m_hWndParent, rcDataArea);
    m_pVerticalNav->Show(FALSE);
}

void CViewManager::SwitchToView(ViewType viewType, const RECT& rcDataArea)
{
    HideAllViews();
    m_currentView = viewType;

    CRect rcList = rcDataArea;

    if (viewType == VIEW_DATA)
    {
        m_pDataView->MoveWindow(rcList);
        m_pDataView->Show();
        m_pDataView->Refresh();
    }
    else if (viewType == VIEW_STATS)
    {
        m_pStatsView->MoveWindow(rcList);
        m_pStatsView->Show();
        m_pStatsView->Refresh();
    }
    else if (viewType == VIEW_PROCESS)
    {
        int navWidth = m_pVerticalNav->GetWidth();
        
        CRect rcNav = rcList;
        rcNav.right = rcNav.left + navWidth;
        
        m_pVerticalNav->MoveWindow(rcNav);
        m_pVerticalNav->Show();

        rcList.left += navWidth;
        m_pDataView->MoveWindow(rcList);
        m_pDataView->Show();
        m_pDataView->Refresh();
    }
}

void CViewManager::LayoutViews(const RECT& rcDataArea)
{
    if (m_currentView == VIEW_NONE)
        return;

    CRect rcList = rcDataArea;

    if (m_currentView == VIEW_DATA)
    {
        m_pDataView->MoveWindow(rcList);
    }
    else if (m_currentView == VIEW_STATS)
    {
        m_pStatsView->MoveWindow(rcList);
    }
    else if (m_currentView == VIEW_PROCESS)
    {
        int navWidth = m_pVerticalNav->GetWidth();
        
        CRect rcNav = rcList;
        rcNav.right = rcNav.left + navWidth;
        m_pVerticalNav->MoveWindow(rcNav);

        rcList.left += navWidth;
        m_pDataView->MoveWindow(rcList);
    }
}

void CViewManager::RefreshCurrentView()
{
    if (m_pDataView)
        m_pDataView->Refresh();
    if (m_pStatsView)
        m_pStatsView->Refresh();
}

void CViewManager::HideAllViews()
{
    if (m_pDataView)
        m_pDataView->Show(FALSE);
    if (m_pStatsView)
        m_pStatsView->Show(FALSE);
    if (m_pVerticalNav)
        m_pVerticalNav->Show(FALSE);
}
