#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int nCmdShow)
{
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
