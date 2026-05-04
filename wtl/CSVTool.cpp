#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

#pragma comment(lib, "comctl32.lib")

CAppModule _Module;

static void InitAppCommonControls()
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    ::InitCommonControlsEx(&iccx);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int nCmdShow)
{
    ::SetConsoleCP(CP_UTF8);
    ::SetConsoleOutputCP(CP_UTF8);

    InitAppCommonControls();

    HRESULT hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ATLASSERT(SUCCEEDED(hRes));

    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    int nRet = 0;
    {
        CMainDlg dlg;
        nRet = dlg.DoModal();
    }

    _Module.Term();
    ::CoUninitialize();

    return nRet;
}
