#include "stdafx.h"
#include "CSVModel.h"

CString CEncodingHelper::AnsiToUnicode(const char* pszAnsi)
{
    if (pszAnsi == NULL || *pszAnsi == '\0')
        return CString();

    int nLen = ::MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, NULL, 0);
    CString strResult;
    ::MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, strResult.GetBuffer(nLen), nLen);
    strResult.ReleaseBuffer();
    return strResult;
}

CString CEncodingHelper::UTF8ToUnicode(const char* pszUTF8)
{
    if (pszUTF8 == NULL || *pszUTF8 == '\0')
        return CString();

    int nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszUTF8, -1, NULL, 0);
    CString strResult;
    ::MultiByteToWideChar(CP_UTF8, 0, pszUTF8, -1, strResult.GetBuffer(nLen), nLen);
    strResult.ReleaseBuffer();
    return strResult;
}

bool CEncodingHelper::IsUTF8WithBOM(const char* pBuffer, size_t nSize)
{
    if (nSize >= 3 && 
        (unsigned char)pBuffer[0] == 0xEF && 
        (unsigned char)pBuffer[1] == 0xBB && 
        (unsigned char)pBuffer[2] == 0xBF)
    {
        return true;
    }
    return false;
}

CCSVModel::CCSVModel()
    : m_bLoaded(false)
{
}

CString CCSVModel::GetHeader(size_t col) const
{
    if (col < m_headers.size())
        return m_headers[col];
    return CString();
}

CString CCSVModel::GetCell(size_t row, size_t col) const
{
    if (row < m_data.size() && col < m_data[row].size())
        return m_data[row][col];
    return CString();
}

void CCSVModel::GetStatistics(std::vector<std::pair<CString, CString>>& stats) const
{
    stats.clear();

    if (!m_bLoaded)
        return;

    CString strTemp;

    strTemp.Format(_T("%d"), (int)m_data.size());
    stats.push_back(std::make_pair(CString(_T("总行数")), strTemp));

    strTemp.Format(_T("%d"), (int)m_headers.size());
    stats.push_back(std::make_pair(CString(_T("总列数")), strTemp));

    size_t nonEmptyCells = 0;
    size_t totalCells = 0;

    for (size_t row = 0; row < m_data.size(); ++row)
    {
        for (size_t col = 0; col < m_data[row].size(); ++col)
        {
            totalCells++;
            if (!m_data[row][col].IsEmpty())
                nonEmptyCells++;
        }
    }

    strTemp.Format(_T("%d"), (int)totalCells);
    stats.push_back(std::make_pair(CString(_T("总单元格数")), strTemp));

    strTemp.Format(_T("%d"), (int)nonEmptyCells);
    stats.push_back(std::make_pair(CString(_T("非空单元格数")), strTemp));

    strTemp.Format(_T("%d"), (int)(totalCells - nonEmptyCells));
    stats.push_back(std::make_pair(CString(_T("空单元格数")), strTemp));

    for (size_t col = 0; col < m_headers.size(); ++col)
    {
        size_t colNonEmpty = 0;
        for (size_t row = 0; row < m_data.size(); ++row)
        {
            if (col < m_data[row].size() && !m_data[row][col].IsEmpty())
                colNonEmpty++;
        }
        CString strName;
        strName.Format(_T("列[%s]非空数"), m_headers[col]);
        strTemp.Format(_T("%d"), (int)colNonEmpty);
        stats.push_back(std::make_pair(strName, strTemp));
    }
}

bool CCSVModel::LoadFromFile(LPCTSTR pszFilePath)
{
    m_bLoaded = false;
    m_data.clear();
    m_headers.clear();

    USES_CONVERSION;

    HANDLE hFile = ::CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwFileSize = ::GetFileSize(hFile, NULL);
    if (dwFileSize == 0 || dwFileSize == INVALID_FILE_SIZE)
    {
        ::CloseHandle(hFile);
        return false;
    }

    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_FIXED, dwFileSize + 2);
    if (hGlobal == NULL)
    {
        ::CloseHandle(hFile);
        return false;
    }

    char* pBuffer = (char*)hGlobal;
    DWORD dwRead = 0;
    ::ReadFile(hFile, pBuffer, dwFileSize, &dwRead, NULL);
    pBuffer[dwRead] = '\0';
    pBuffer[dwRead + 1] = '\0';

    ::CloseHandle(hFile);

    int nStartPos = 0;
    if (CEncodingHelper::IsUTF8WithBOM(pBuffer, dwFileSize))
    {
        nStartPos = 3;
    }

    std::string strContent(&pBuffer[nStartPos]);
    ::GlobalFree(hGlobal);

    bool bIsUTF8 = IsContentUTF8(strContent.c_str(), strContent.size());
    
    return ParseCSVContent(strContent, bIsUTF8);
}

bool CCSVModel::IsContentUTF8(const char* pBuffer, size_t nSize)
{
    size_t i = 0;
    while (i < nSize)
    {
        if ((unsigned char)pBuffer[i] <= 0x7F)
        {
            i++;
        }
        else if ((unsigned char)pBuffer[i] >= 0xC0 && (unsigned char)pBuffer[i] <= 0xDF)
        {
            if (i + 1 >= nSize) return false;
            if ((unsigned char)pBuffer[i + 1] < 0x80) return false;
            i += 2;
        }
        else if ((unsigned char)pBuffer[i] >= 0xE0 && (unsigned char)pBuffer[i] <= 0xEF)
        {
            if (i + 2 >= nSize) return false;
            if ((unsigned char)pBuffer[i + 1] < 0x80 || (unsigned char)pBuffer[i + 2] < 0x80) return false;
            i += 3;
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool CCSVModel::ParseCSVContent(const std::string& strContent, bool bIsUTF8)
{
    std::string::size_type nPos = 0;
    bool bFirstLine = true;

    while (nPos < strContent.size())
    {
        std::string::size_type nLineEnd = strContent.find('\n', nPos);
        if (nLineEnd == std::string::npos)
            nLineEnd = strContent.size();

        std::string strLine = strContent.substr(nPos, nLineEnd - nPos);
        
        if (!strLine.empty() && strLine[strLine.size() - 1] == '\r')
            strLine = strLine.substr(0, strLine.size() - 1);

        if (!strLine.empty())
        {
            std::vector<CString> row;
            ParseCSVLine(strLine, row, bIsUTF8);

            if (bFirstLine)
            {
                m_headers = row;
                bFirstLine = false;
            }
            else
            {
                m_data.push_back(row);
            }
        }

        nPos = nLineEnd + 1;
    }

    m_bLoaded = true;
    return true;
}

void CCSVModel::ParseCSVLine(const std::string& strLine, std::vector<CString>& row, bool bIsUTF8)
{
    std::string::size_type nStart = 0;
    bool bInQuotes = false;
    std::string strField;

    for (std::string::size_type i = 0; i <= strLine.size(); ++i)
    {
        char c = (i < strLine.size()) ? strLine[i] : '\0';

        if (c == '"')
        {
            if (bInQuotes && i + 1 < strLine.size() && strLine[i + 1] == '"')
            {
                strField += '"';
                ++i;
            }
            else
            {
                bInQuotes = !bInQuotes;
            }
        }
        else if ((c == ',' && !bInQuotes) || c == '\0')
        {
            CString strValue;
            if (bIsUTF8)
                strValue = CEncodingHelper::UTF8ToUnicode(strField.c_str());
            else
                strValue = CEncodingHelper::AnsiToUnicode(strField.c_str());
            
            row.push_back(strValue);
            strField.clear();
        }
        else
        {
            strField += c;
        }
    }
}
