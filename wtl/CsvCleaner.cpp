#include "stdafx.h"
#include "CsvCleaner.h"
#include <algorithm>
#include <set>

CCsvCleaner::CCsvCleaner()
    : m_pReader(NULL)
{
}

void CCsvCleaner::SetReader(CCsvReader* pReader)
{
    m_pReader = pReader;
}

int CCsvCleaner::GetColumnIndex(const CString& columnName) const
{
    if (!m_pReader)
        return -1;

    const auto& headers = m_pReader->GetHeaders();
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (headers[i] == columnName)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

CString CCsvCleaner::TrimLeft(const CString& s) const
{
    CString result = s;
    result.TrimLeft();
    return result;
}

CString CCsvCleaner::TrimRight(const CString& s) const
{
    CString result = s;
    result.TrimRight();
    return result;
}

CString CCsvCleaner::Trim(const CString& s) const
{
    CString result = s;
    result.Trim();
    return result;
}

bool CCsvCleaner::IsMissingValue(const CString& value) const
{
    CString trimmed = Trim(value);
    return trimmed.IsEmpty() || 
           trimmed == _T("NA") || 
           trimmed == _T("N/A") || 
           trimmed == _T("null") || 
           trimmed == _T("NULL");
}

void CCsvCleaner::TrimWhitespace()
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    auto& data = m_pReader->GetData();
    for (auto& row : data)
    {
        for (auto& field : row)
        {
            field = Trim(field);
        }
    }
}

void CCsvCleaner::TrimWhitespaceInColumn(int columnIndex)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pReader->GetColumnCount())
        return;
    
    auto& data = m_pReader->GetData();
    for (auto& row : data)
    {
        if (static_cast<size_t>(columnIndex) < row.size())
        {
            row[columnIndex] = Trim(row[columnIndex]);
        }
    }
}

void CCsvCleaner::TrimWhitespaceInColumn(const CString& columnName)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        TrimWhitespaceInColumn(idx);
    }
}

void CCsvCleaner::RemoveRowsWithMissingValues()
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    auto& data = m_pReader->GetData();
    std::vector<std::vector<CString>> newData;
    
    for (const auto& row : data)
    {
        bool hasMissing = false;
        for (const auto& field : row)
        {
            if (IsMissingValue(field))
            {
                hasMissing = true;
                break;
            }
        }
        if (!hasMissing)
        {
            newData.push_back(row);
        }
    }
    
    m_pReader->SetData(newData);
}

void CCsvCleaner::RemoveRowsWithMissingValuesInColumn(int columnIndex)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pReader->GetColumnCount())
        return;
    
    auto& data = m_pReader->GetData();
    std::vector<std::vector<CString>> newData;
    
    for (const auto& row : data)
    {
        if (static_cast<size_t>(columnIndex) < row.size())
        {
            if (!IsMissingValue(row[columnIndex]))
            {
                newData.push_back(row);
            }
        }
    }
    
    m_pReader->SetData(newData);
}

void CCsvCleaner::RemoveRowsWithMissingValuesInColumn(const CString& columnName)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        RemoveRowsWithMissingValuesInColumn(idx);
    }
}

void CCsvCleaner::RemoveDuplicateRows()
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    auto& data = m_pReader->GetData();
    std::vector<std::vector<CString>> newData;
    std::set<CString> seen;
    
    for (const auto& row : data)
    {
        CString key;
        for (size_t i = 0; i < row.size(); ++i)
        {
            if (i > 0)
                key += _T("\x1F");
            key += row[i];
        }
        
        if (seen.find(key) == seen.end())
        {
            seen.insert(key);
            newData.push_back(row);
        }
    }
    
    m_pReader->SetData(newData);
}

void CCsvCleaner::StandardizeDateFormats(const CString& targetFormat)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    size_t colCount = m_pReader->GetColumnCount();
    for (size_t i = 0; i < colCount; ++i)
    {
        StandardizeDateFormatsInColumn(static_cast<int>(i), targetFormat);
    }
}

bool CCsvCleaner::IsDateSeparator(TCHAR c) const
{
    return c == _T('-') || c == _T('/') || c == _T('.');
}

bool CCsvCleaner::ParseDateSimple(const CString& value, int& year, int& month, int& day) const
{
    CString trimmed = value;
    trimmed.Trim();
    
    if (trimmed.IsEmpty())
        return false;

    int len = trimmed.GetLength();
    int numbers[3] = {0, 0, 0};
    int numCount = 0;
    int currentNum = 0;

    for (int i = 0; i < len; ++i)
    {
        TCHAR c = trimmed[i];
        if (c >= _T('0') && c <= _T('9'))
        {
            currentNum = currentNum * 10 + (c - _T('0'));
        }
        else if (IsDateSeparator(c) || c == _T(' ') || c == _T(','))
        {
            if (numCount < 3)
            {
                numbers[numCount++] = currentNum;
                currentNum = 0;
            }
        }
        else if (c < _T('0') || c > _T('9'))
        {
            if (numCount < 3)
            {
                numbers[numCount++] = currentNum;
                currentNum = 0;
            }
            break;
        }
    }
    if (numCount < 3 && currentNum > 0)
    {
        numbers[numCount++] = currentNum;
    }

    if (numCount < 3)
    {
        if (len == 8)
        {
            numbers[0] = _ttoi(trimmed.Left(4));
            numbers[1] = _ttoi(trimmed.Mid(4, 2));
            numbers[2] = _ttoi(trimmed.Right(2));
            numCount = 3;
        }
    }

    if (numCount != 3)
        return false;

    int n1 = numbers[0];
    int n2 = numbers[1];
    int n3 = numbers[2];

    if (n1 >= 1900 && n1 <= 2100 && n2 >= 1 && n2 <= 12 && n3 >= 1 && n3 <= 31)
    {
        year = n1;
        month = n2;
        day = n3;
        return true;
    }

    if (n3 >= 1900 && n3 <= 2100 && n2 >= 1 && n2 <= 12 && n1 >= 1 && n1 <= 31)
    {
        year = n3;
        month = n2;
        day = n1;
        return true;
    }

    return false;
}

void CCsvCleaner::StandardizeDateFormatsInColumn(int columnIndex, const CString& targetFormat)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pReader->GetColumnCount())
        return;
    
    auto& data = m_pReader->GetData();
    for (auto& row : data)
    {
        if (static_cast<size_t>(columnIndex) < row.size())
        {
            CString value = row[columnIndex];
            value.Trim();
            
            int year = 0, month = 0, day = 0;
            if (ParseDateSimple(value, year, month, day))
            {
                CString formatted;
                if (targetFormat == _T("%Y-%m-%d"))
                {
                    formatted.Format(_T("%04d-%02d-%02d"), year, month, day);
                }
                else if (targetFormat == _T("%Y/%m/%d"))
                {
                    formatted.Format(_T("%04d/%02d/%02d"), year, month, day);
                }
                else if (targetFormat == _T("%d/%m/%Y"))
                {
                    formatted.Format(_T("%02d/%02d/%04d"), day, month, year);
                }
                else if (targetFormat == _T("%m/%d/%Y"))
                {
                    formatted.Format(_T("%02d/%02d/%04d"), month, day, year);
                }
                else
                {
                    formatted.Format(_T("%04d-%02d-%02d"), year, month, day);
                }
                row[columnIndex] = formatted;
            }
        }
    }
}

void CCsvCleaner::StandardizeDateFormatsInColumn(const CString& columnName, const CString& targetFormat)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        StandardizeDateFormatsInColumn(idx, targetFormat);
    }
}
