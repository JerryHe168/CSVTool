#pragma once
#include "stdafx.h"
#include "CsvReader.h"

class CCsvCleaner
{
protected:
    CCsvReader* m_pReader;

public:
    CCsvCleaner();
    virtual ~CCsvCleaner() {}

    void SetReader(CCsvReader* pReader);
    CCsvReader* GetReader() const { return m_pReader; }
    
    CString TrimLeft(const CString& s) const;
    CString TrimRight(const CString& s) const;
    CString Trim(const CString& s) const;
    bool IsMissingValue(const CString& value) const;
    
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

protected:
    int GetColumnIndex(const CString& columnName) const;
};
