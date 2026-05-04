#pragma once
#include "stdafx.h"

class CCsvReader
{
protected:
    char m_delimiter;
    std::vector<std::vector<CString>> m_data;
    std::vector<CString> m_headers;
    bool m_bLoaded;

public:
    CCsvReader();
    virtual ~CCsvReader() {}

    bool Read(LPCTSTR pszFilePath, char delimiter = ',');
    bool IsLoaded() const { return m_bLoaded; }
    
    size_t GetRowCount() const { return m_data.size(); }
    size_t GetColumnCount() const { return m_headers.size(); }
    char GetDelimiter() const { return m_delimiter; }
    
    CString GetHeader(size_t col) const;
    CString GetCell(size_t row, size_t col) const;
    
    const std::vector<CString>& GetHeaders() const { return m_headers; }
    std::vector<CString>& GetHeaders() { return m_headers; }
    const std::vector<std::vector<CString>>& GetData() const { return m_data; }
    std::vector<std::vector<CString>>& GetData() { return m_data; }
    
    void SetCell(size_t row, size_t col, const CString& value);
    void SetData(const std::vector<std::vector<CString>>& data);
    void SetHeaders(const std::vector<CString>& headers);
    void Clear();

protected:
    static bool IsUTF8WithBOM(const char* pBuffer, size_t nSize);
    static bool IsContentUTF8(const char* pBuffer, size_t nSize);
    static CString AnsiToUnicode(const char* pszAnsi);
    static CString UTF8ToUnicode(const char* pszUTF8);
    
    std::vector<CString> ParseCSVLine(const std::string& strLine, bool bIsUTF8);
};
