#pragma once
#include "stdafx.h"
#include "CsvReader.h"

class CCsvConverter
{
protected:
    CCsvReader* m_pReader;

public:
    CCsvConverter();
    virtual ~CCsvConverter() {}

    void SetReader(CCsvReader* pReader);
    CCsvReader* GetReader() const { return m_pReader; }
    
    CString Trim(const CString& s) const;
    int CompareValues(const CString& a, const CString& b) const;
    int GetColumnIndex(const CString& columnName) const;
    
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
