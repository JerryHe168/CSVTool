#pragma once
#include "stdafx.h"
#include "ViewInterfaces.h"

class CEncodingHelper
{
public:
    static CString AnsiToUnicode(const char* pszAnsi);
    static CString UTF8ToUnicode(const char* pszUTF8);
    static bool IsUTF8WithBOM(const char* pBuffer, size_t nSize);
};

class CCSVModel : public IDataModel
{
protected:
    std::vector<std::vector<CString>> m_data;
    std::vector<CString> m_headers;
    bool m_bLoaded;

public:
    CCSVModel();
    virtual ~CCSVModel() {}

    virtual bool IsLoaded() const { return m_bLoaded; }
    virtual size_t GetRowCount() const { return m_data.size(); }
    virtual size_t GetColumnCount() const { return m_headers.size(); }
    
    virtual CString GetHeader(size_t col) const;
    virtual CString GetCell(size_t row, size_t col) const;
    virtual void GetStatistics(std::vector<std::pair<CString, CString>>& stats) const;
    virtual bool LoadFromFile(LPCTSTR pszFilePath);

protected:
    bool IsContentUTF8(const char* pBuffer, size_t nSize);
    bool ParseCSVContent(const std::string& strContent, bool bIsUTF8);
    void ParseCSVLine(const std::string& strLine, std::vector<CString>& row, bool bIsUTF8);
};
