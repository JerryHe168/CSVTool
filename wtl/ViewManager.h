#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"
#include "NavigationBar.h"

class CDataListView;
class CStatsListView;
class CVerticalNavBar;

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
    virtual ~CViewManager() {}

    void SetModel(IDataModel* pModel);
    void SetParent(HWND hWndParent);
    ViewType GetCurrentView() const;

    void CreateViews(const RECT& rcDataArea);
    void SwitchToView(ViewType viewType, const RECT& rcDataArea);
    void LayoutViews(const RECT& rcDataArea);
    void RefreshCurrentView();

protected:
    void HideAllViews();
};
