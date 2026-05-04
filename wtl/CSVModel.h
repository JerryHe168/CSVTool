#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"
#include "CsvReader.h"
#include "CsvWriter.h"
#include "CsvStatistics.h"
#include "CsvCleaner.h"
#include <map>

class CCSVModel : public IDataModel
{
protected:
    CCsvReader m_reader;
    CCsvWriter m_writer;
    CCsvStatistics m_statistics;
    CCsvCleaner m_cleaner;
    CString m_currentFilePath;
    mutable std::map<CString, int> m_columnIndexCache;

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
    
    void SetCell(size_t row, size_t col, const CString& value);
    int GetColumnIndex(const CString& columnName) const;
    CString GetColumnName(int columnIndex) const;
    void InvalidateColumnIndexCache() const;
    void UpdateStatistics();
    
    CString TrimLeft(const CString& s) const;
    CString TrimRight(const CString& s) const;
    CString Trim(const CString& s) const;
    bool IsMissingValue(const CString& value) const;
    int CompareValues(const CString& a, const CString& b) const;
    
    void TrimWhitespace();
    void TrimWhitespaceInColumn(int columnIndex);
    void TrimWhitespaceInColumn(const CString& columnName);
    
    void RemoveRowsWithMissingValues();
    void RemoveRowsWithMissingValuesInColumn(int columnIndex);
    void RemoveRowsWithMissingValuesInColumn(const CString& columnName);
    
    void RemoveDuplicateRows();
    
    void StandardizeDateFormats(const CString& targetFormat);
    void StandardizeDateFormatsInColumn(int columnIndex, const CString& targetFormat);
    void StandardizeDateFormatsInColumn(const CString& columnName, const CString& targetFormat);
    
    bool AddColumn(int position, const CString& columnName, const CString& defaultValue);
    bool AddColumn(const CString& columnName, const CString& defaultValue);
    bool RemoveColumn(int columnIndex);
    bool RemoveColumn(const CString& columnName);
    bool RenameColumn(int columnIndex, const CString& newName);
    bool RenameColumn(const CString& oldName, const CString& newName);
    
    void ReplaceString(const CString& search, const CString& replace);
    void ReplaceStringInColumn(int columnIndex, const CString& search, const CString& replace);
    void ReplaceStringInColumn(const CString& columnName, const CString& search, const CString& replace);
    
    void SortByColumn(int columnIndex, bool ascending);
    void SortByColumn(const CString& columnName, bool ascending);
};
