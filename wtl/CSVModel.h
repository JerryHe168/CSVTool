#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"
#include "CsvReader.h"
#include "CsvWriter.h"
#include "CsvStatistics.h"

class CCSVModel : public IDataModel
{
protected:
    CCsvReader m_reader;
    CCsvWriter m_writer;
    CCsvStatistics m_statistics;
    CString m_currentFilePath;

public:
    CCSVModel();
    virtual ~CCSVModel() {}

    virtual bool IsLoaded() const { return m_reader.IsLoaded(); }
    virtual size_t GetRowCount() const { return m_reader.GetRowCount(); }
    virtual size_t GetColumnCount() const { return m_reader.GetColumnCount(); }
    
    virtual CString GetHeader(size_t col) const;
    virtual CString GetCell(size_t row, size_t col) const;
    virtual void GetStatistics(std::vector<std::pair<CString, CString>>& stats) const;
    virtual bool LoadFromFile(LPCTSTR pszFilePath);
    virtual bool SaveToFile(LPCTSTR pszFilePath);
    virtual bool SaveToFileUTF8(LPCTSTR pszFilePath);
    
    virtual const std::vector<CString>& GetHeaders() const { return m_reader.GetHeaders(); }
    virtual const std::vector<std::vector<CString>>& GetData() const { return m_reader.GetData(); }
    virtual CColumnStatistics GetColumnStatistics(int columnIndex) const;
    virtual CColumnStatistics GetColumnStatistics(const CString& columnName) const;
    virtual std::map<CString, CColumnStatistics> GetAllColumnStatistics() const;
    
    CString GetCurrentFilePath() const { return m_currentFilePath; }
    void SetCurrentFilePath(LPCTSTR pszPath) { m_currentFilePath = pszPath; }
};
