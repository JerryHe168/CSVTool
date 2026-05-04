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

void CMainDlg::RefreshCurrentView()
{
    m_viewManager.RefreshCurrentView();
}

LRESULT CMainDlg::OnBtnClean(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_model.IsLoaded())
    {
        ::MessageBox(m_hWnd, _T("请先打开一个CSV文件！"), _T("提示"), MB_OK | MB_ICONWARNING);
        return 0;
    }

    CString strMsg = _T("是否执行全部数据清洗操作？\n\n")
        _T("将执行以下操作：\n")
        _T("1. 去除所有字段的空白字符\n")
        _T("2. 删除包含缺失值的行\n")
        _T("3. 删除重复数据行\n")
        _T("4. 标准化日期格式为 YYYY-MM-DD\n\n")
        _T("点击【是】执行全部清洗，点击【否】仅去除空白字符。");

    int nResult = ::MessageBox(m_hWnd, strMsg, _T("数据清洗"), MB_YESNOCANCEL | MB_ICONQUESTION);
    
    if (nResult == IDCANCEL)
    {
        return 0;
    }

    CString strResult;
    
    if (nResult == IDYES)
    {
        m_model.TrimWhitespace();
        m_model.RemoveRowsWithMissingValues();
        m_model.RemoveDuplicateRows();
        m_model.StandardizeDateFormats(_T("%Y-%m-%d"));
        strResult = _T("已执行全部清洗操作！\n\n")
            _T("- 去除空白字符\n")
            _T("- 删除缺失值行\n")
            _T("- 删除重复行\n")
            _T("- 标准化日期格式");
    }
    else
    {
        m_model.TrimWhitespace();
        strResult = _T("已去除所有字段的空白字符！");
    }

    RefreshCurrentView();
    ::MessageBox(m_hWnd, strResult, _T("数据清洗完成"), MB_OK | MB_ICONINFORMATION);
    return 0;
}

LRESULT CMainDlg::OnBtnConvert(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_model.IsLoaded())
    {
        ::MessageBox(m_hWnd, _T("请先打开一个CSV文件！"), _T("提示"), MB_OK | MB_ICONWARNING);
        return 0;
    }

    CString strMsg = _T("数据转换功能说明：\n\n")
        _T("本功能支持以下操作：\n")
        _T("1. 添加新列\n")
        _T("2. 删除列\n")
        _T("3. 重命名列\n")
        _T("4. 字符串替换\n\n")
        _T("此功能需要进一步完善界面交互。\n")
        _T("当前可以使用数据清洗功能处理数据。");

    ::MessageBox(m_hWnd, strMsg, _T("数据转换"), MB_OK | MB_ICONINFORMATION);
    return 0;
}

LRESULT CMainDlg::OnBtnSort(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_model.IsLoaded())
    {
        ::MessageBox(m_hWnd, _T("请先打开一个CSV文件！"), _T("提示"), MB_OK | MB_ICONWARNING);
        return 0;
    }

    CString strMsg = _T("数据排序功能：\n\n")
        _T("本功能支持按指定列进行升序或降序排序。\n\n")
        _T("此功能需要进一步完善界面交互。\n")
        _T("当前可以使用数据清洗功能处理数据。");

    ::MessageBox(m_hWnd, strMsg, _T("排序数据"), MB_OK | MB_ICONINFORMATION);
    return 0;
}

LRESULT CMainDlg::OnBtnStatProcess(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SwitchToView(VIEW_STATS);
    return 0;
}

LRESULT CMainDlg::OnBtnFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ::MessageBox(m_hWnd, _T("数据筛选功能待实现"), _T("提示"), MB_OK);
    return 0;
}
