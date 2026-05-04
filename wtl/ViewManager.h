#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"
#include "NavigationBar.h"
#include "DataListView.h"
#include "StatsListView.h"

class CViewManager
{
public:
    enum { MARGIN = 10 };

protected:
    std::unique_ptr<CDataListView> m_pDataView;
    std::unique_ptr<CStatsListView> m_pStatsView;
    std::unique_ptr<CVerticalNavBar> m_pVerticalNav;
    
    IDataModel* m_pModel;
    HWND m_hWndParent;
    ViewType m_currentView;

public:
    CViewManager();
    ~CViewManager();

    void SetModel(IDataModel* pModel);
    void SetParent(HWND hWndParent);
    ViewType GetCurrentView() const;

    void CreateViews(RECT& rcDataArea);
    void SwitchToView(ViewType viewType, RECT& rcDataArea);
    void LayoutViews(RECT& rcDataArea);
    void RefreshCurrentView();

protected:
    void HideAllViews();
};
