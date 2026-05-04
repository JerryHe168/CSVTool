#include "stdafx.h"
#include "CsvWriter.h"

CCsvWriter::CCsvWriter()
    : m_delimiter(',')
{
}

std::string CCsvWriter::CStringToUTF8(const CString& str)
{
    if (str.IsEmpty())
        return "";

    int nLen = ::WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (nLen <= 0)
        return "";

    std::string result(nLen - 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, str, -1, &result[0], nLen, NULL, NULL);
    return result;
}

std::string CCsvWriter::CStringToAnsi(const CString& str)
{
    if (str.IsEmpty())
        return "";

    int nLen = ::WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    if (nLen <= 0)
        return "";

    std::string result(nLen - 1, '\0');
    ::WideCharToMultiByte(CP_ACP, 0, str, -1, &result[0], nLen, NULL, NULL);
    return result;
}

std::string CCsvWriter::FormatCSVField(const std::string& field, char delimiter)
{
    bool needsQuotes = false;
    for (char c : field)
    {
        if (c == delimiter || c == '"' || c == '\n' || c == '\r')
        {
            needsQuotes = true;
            break;
        }
    }

    if (!needsQuotes)
    {
        return field;
    }

    std::string result = "\"";
    for (char c : field)
    {
        if (c == '"')
        {
            result += "\"\"";
        }
        else
        {
            result += c;
        }
    }
    result += "\"";
    return result;
}

bool CCsvWriter::Write(LPCTSTR pszFilePath,
                        const std::vector<CString>& headers,
                        const std::vector<std::vector<CString>>& data,
                        char delimiter)
{
    m_delimiter = delimiter;

    USES_CONVERSION;
    std::ofstream file(T2CA(pszFilePath), std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (i > 0)
            file << delimiter;
        std::string field = CStringToUTF8(headers[i]);
        file << FormatCSVField(field, delimiter);
    }
    file << "\r\n";

    for (const auto& row : data)
    {
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (i > 0)
                file << delimiter;
            if (i < row.size())
            {
                std::string field = CStringToUTF8(row[i]);
                file << FormatCSVField(field, delimiter);
            }
        }
        file << "\r\n";
    }

    file.close();
    return true;
}

bool CCsvWriter::WriteUTF8(LPCTSTR pszFilePath,
                            const std::vector<CString>& headers,
                            const std::vector<std::vector<CString>>& data,
                            char delimiter)
{
    m_delimiter = delimiter;

    USES_CONVERSION;
    std::ofstream file(T2CA(pszFilePath), std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    const unsigned char utf8BOM[] = {0xEF, 0xBB, 0xBF};
    file.write(reinterpret_cast<const char*>(utf8BOM), 3);

    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (i > 0)
            file << delimiter;
        std::string field = CStringToUTF8(headers[i]);
        file << FormatCSVField(field, delimiter);
    }
    file << "\r\n";

    for (const auto& row : data)
    {
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (i > 0)
                file << delimiter;
            if (i < row.size())
            {
                std::string field = CStringToUTF8(row[i]);
                file << FormatCSVField(field, delimiter);
            }
        }
        file << "\r\n";
    }

    file.close();
    return true;
}
