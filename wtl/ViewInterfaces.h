#pragma once
#include "stdafx.h"

struct CColumnStatistics;
class IDataModel;

enum ViewType
{
    VIEW_NONE = 0,
    VIEW_DATA = 1,
    VIEW_STATS = 2,
    VIEW_PROCESS = 3
};

class IView
{
public:
    virtual ~IView() = 0 {}
    
    virtual HWND Create(HWND hWndParent, RECT& rc) = 0;
    virtual void Show(BOOL bShow = TRUE) = 0;
    virtual void MoveWindow(const RECT& rc) = 0;
    virtual HWND GetHwnd() const = 0;
    virtual ViewType GetViewType() const = 0;
    virtual void Refresh() = 0;
    virtual void SetModel(IDataModel* pModel) = 0;
};

class IDataModel
{
public:
    virtual ~IDataModel() = 0 {}
    
    virtual bool IsLoaded() const = 0;
    virtual size_t GetRowCount() const = 0;
    virtual size_t GetColumnCount() const = 0;
    virtual CString GetHeader(size_t col) const = 0;
    virtual CString GetCell(size_t row, size_t col) const = 0;
    virtual void GetStatistics(std::vector<std::pair<CString, CString>>& stats) const = 0;
    virtual bool LoadFromFile(LPCTSTR pszFilePath) = 0;
    virtual bool SaveToFile(LPCTSTR pszFilePath) = 0;
    virtual bool SaveToFileUTF8(LPCTSTR pszFilePath) = 0;
    
    virtual const std::vector<CString>& GetHeaders() const = 0;
    virtual const std::vector<std::vector<CString>>& GetData() const = 0;
    virtual CColumnStatistics GetColumnStatistics(int columnIndex) const = 0;
    virtual CColumnStatistics GetColumnStatistics(const CString& columnName) const = 0;
    virtual std::map<CString, CColumnStatistics> GetAllColumnStatistics() const = 0;
};

class CViewBase : public IView
{
protected:
    IDataModel* m_pModel;
    
public:
    CViewBase();
    virtual ~CViewBase() {}
    
    virtual void SetModel(IDataModel* pModel) { m_pModel = pModel; }
};

class INavigationBar
{
public:
    virtual ~INavigationBar() = 0 {}
    
    virtual HWND Create(HWND hWndParent, RECT& rc) = 0;
    virtual void Show(BOOL bShow = TRUE) = 0;
    virtual void MoveWindow(const RECT& rc) = 0;
    virtual HWND GetHwnd() const = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
};
