#pragma once
#include "stdafx.h"

class CCsvWriter
{
protected:
    char m_delimiter;

public:
    CCsvWriter();
    virtual ~CCsvWriter() {}

    bool Write(LPCTSTR pszFilePath, 
               const std::vector<CString>& headers,
               const std::vector<std::vector<CString>>& data,
               char delimiter = ',');
    
    bool WriteUTF8(LPCTSTR pszFilePath,
                   const std::vector<CString>& headers,
                   const std::vector<std::vector<CString>>& data,
                   char delimiter = ',');

protected:
    static std::string CStringToUTF8(const CString& str);
    static std::string CStringToAnsi(const CString& str);
    static std::string FormatCSVField(const std::string& field, char delimiter);
};
