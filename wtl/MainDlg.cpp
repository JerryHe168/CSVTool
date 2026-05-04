#include "stdafx.h"
#include "MainDlg.h"

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow();

    SetWindowText(_T("CSV Tool - 数据处理工具"));

    m_topNavBar.Create(m_hWnd);
    m_viewManager.SetModel(&m_model);
    m_viewManager.SetParent(m_hWnd);

    CRect rcInitial;
    GetDataAreaRect(rcInitial);
    m_viewManager.CreateViews(rcInitial);

    LayoutControls();

    return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = FALSE;
    return 0;
}

LRESULT CMainDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    LayoutControls();
    return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CMainDlg::OnBtnOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CFileDialog dlg(TRUE, _T("csv"), NULL, 
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
        _T("CSV文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0"), m_hWnd);

    if (dlg.DoModal() == IDOK)
    {
        CString strPath = dlg.m_szFileName;
        
        if (m_model.LoadFromFile(strPath))
        {
            SwitchToView(VIEW_DATA);
            ::MessageBox(m_hWnd, _T("文件打开成功！"), _T("提示"), 
                MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            ::MessageBox(m_hWnd, _T("文件打开失败！"), _T("错误"), 
                MB_OK | MB_ICONERROR);
        }
    }
    return 0;
}

LRESULT CMainDlg::OnBtnSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_model.IsLoaded())
    {
        ::MessageBox(m_hWnd, _T("请先打开一个CSV文件！"), _T("提示"), MB_OK | MB_ICONWARNING);
        return 0;
    }

    CString strCurrentPath = m_model.GetCurrentFilePath();
    CString strSavePath;

    if (!strCurrentPath.IsEmpty())
    {
        strSavePath = strCurrentPath;
    }
    else
    {
        CFileDialog dlg(FALSE, _T("csv"), NULL, 
            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
            _T("CSV文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0"), m_hWnd);

        if (dlg.DoModal() != IDOK)
        {
            return 0;
        }

        strSavePath = dlg.m_szFileName;
        
        CString strExt = ::PathFindExtension(strSavePath);
        if (strExt.IsEmpty())
        {
            strSavePath += _T(".csv");
        }
    }

    if (m_model.SaveToFileUTF8(strSavePath))
    {
        ::MessageBox(m_hWnd, _T("文件保存成功！"), _T("提示"), 
            MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        ::MessageBox(m_hWnd, _T("文件保存失败！"), _T("错误"), 
            MB_OK | MB_ICONERROR);
    }

    return 0;
}

LRESULT CMainDlg::OnBtnStats(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SwitchToView(VIEW_STATS);
    return 0;
}

LRESULT CMainDlg::OnBtnProcess(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SwitchToView(VIEW_PROCESS);
    return 0;
}

LRESULT CMainDlg::OnBtnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ::MessageBox(m_hWnd, _T("导出功能待实现"), _T("提示"), MB_OK);
    return 0;
}

void CMainDlg::LayoutControls()
{
    CRect rcClient;
    GetClientRect(&rcClient);

    m_topNavBar.Layout(rcClient);

    CRect rcDataArea;
    GetDataAreaRect(rcDataArea);
    
    m_viewManager.LayoutViews(rcDataArea);
}

void CMainDlg::GetDataAreaRect(CRect& rc)
{
    GetClientRect(&rc);
    rc.top = CHorizontalNavBar::MARGIN * 2 + CHorizontalNavBar::BTN_SIZE;
    rc.DeflateRect(MARGIN, MARGIN, MARGIN, MARGIN);
}

void CMainDlg::SwitchToView(ViewType viewType)
{
    CRect rcDataArea;
    GetDataAreaRect(rcDataArea);
    m_viewManager.SwitchToView(viewType, rcDataArea);
}
