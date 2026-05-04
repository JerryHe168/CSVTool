#pragma once
#include "stdafx.h"
#include "resource.h"
#include "ViewInterfaces.h"
#include "CSVModel.h"
#include "NavigationBar.h"
#include "ViewManager.h"

class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
    enum { IDD = IDD_CSVTOOL_DIALOG };
    enum { MARGIN = 10 };

protected:
    CCSVModel m_model;
    CHorizontalNavBar m_topNavBar;
    CViewManager m_viewManager;

public:
    BEGIN_MSG_MAP(CMainDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_HANDLER(IDC_BTN_OPEN, BN_CLICKED, OnBtnOpen)
        COMMAND_HANDLER(IDC_BTN_SAVE, BN_CLICKED, OnBtnSave)
        COMMAND_HANDLER(IDC_BTN_STATS, BN_CLICKED, OnBtnStats)
        COMMAND_HANDLER(IDC_BTN_PROCESS, BN_CLICKED, OnBtnProcess)
        COMMAND_HANDLER(IDC_BTN_EXPORT, BN_CLICKED, OnBtnExport)
        COMMAND_HANDLER(IDC_BTN_CLEAN, BN_CLICKED, OnBtnClean)
        COMMAND_HANDLER(IDC_BTN_CONVERT, BN_CLICKED, OnBtnConvert)
        COMMAND_HANDLER(IDC_BTN_SORT, BN_CLICKED, OnBtnSort)
        COMMAND_HANDLER(IDC_BTN_STAT_PROCESS, BN_CLICKED, OnBtnStatProcess)
        COMMAND_HANDLER(IDC_BTN_FILTER, BN_CLICKED, OnBtnFilter)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnStats(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnProcess(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnExport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnClean(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnConvert(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnSort(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnStatProcess(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBtnFilter(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

protected:
    void LayoutControls();
    void GetDataAreaRect(CRect& rc);
    void SwitchToView(ViewType viewType);
    void RefreshCurrentView();
};
