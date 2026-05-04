#include "stdafx.h"
#include "CsvConverter.h"
#include <algorithm>

CCsvConverter::CCsvConverter()
    : m_pReader(NULL)
{
}

void CCsvConverter::SetReader(CCsvReader* pReader)
{
    m_pReader = pReader;
}

int CCsvConverter::GetColumnIndex(const CString& columnName) const
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

CString CCsvConverter::Trim(const CString& s) const
{
    CString result = s;
    result.Trim();
    return result;
}

int CCsvConverter::CompareValues(const CString& a, const CString& b) const
{
    try
    {
        CString strA = Trim(a);
        CString strB = Trim(b);
        
        bool aIsNumber = !strA.IsEmpty();
        bool bIsNumber = !strB.IsEmpty();
        
        if (aIsNumber && bIsNumber)
        {
            double numA = _ttof((LPCTSTR)strA);
            double numB = _ttof((LPCTSTR)strB);
            
            if (numA < numB) return -1;
            if (numA > numB) return 1;
            return 0;
        }
    }
    catch (...)
    {
    }
    
    return a.Compare(b);
}

bool CCsvConverter::AddColumn(int position, const CString& columnName, const CString& defaultValue)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return false;
    
    if (position < 0 || static_cast<size_t>(position) > m_pReader->GetColumnCount())
        return false;
    
    auto& headers = m_pReader->GetHeaders();
    auto& data = m_pReader->GetData();
    
    headers.insert(headers.begin() + position, columnName);
    
    for (auto& row : data)
    {
        if (static_cast<size_t>(position) <= row.size())
        {
            row.insert(row.begin() + position, defaultValue);
        }
        else
        {
            while (row.size() < static_cast<size_t>(position))
            {
                row.push_back(_T(""));
            }
            row.push_back(defaultValue);
        }
    }
    
    return true;
}

bool CCsvConverter::AddColumn(const CString& columnName, const CString& defaultValue)
{
    if (!m_pReader)
        return false;
    return AddColumn(static_cast<int>(m_pReader->GetColumnCount()), columnName, defaultValue);
}

bool CCsvConverter::RemoveColumn(int columnIndex)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return false;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pReader->GetColumnCount())
        return false;
    
    auto& headers = m_pReader->GetHeaders();
    auto& data = m_pReader->GetData();
    
    headers.erase(headers.begin() + columnIndex);
    
    for (auto& row : data)
    {
        if (static_cast<size_t>(columnIndex) < row.size())
        {
            row.erase(row.begin() + columnIndex);
        }
    }
    
    return true;
}

bool CCsvConverter::RemoveColumn(const CString& columnName)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        return RemoveColumn(idx);
    }
    return false;
}

bool CCsvConverter::RenameColumn(int columnIndex, const CString& newName)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return false;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pReader->GetColumnCount())
        return false;
    
    auto& headers = m_pReader->GetHeaders();
    headers[columnIndex] = newName;
    
    return true;
}

bool CCsvConverter::RenameColumn(const CString& oldName, const CString& newName)
{
    int idx = GetColumnIndex(oldName);
    if (idx >= 0)
    {
        return RenameColumn(idx, newName);
    }
    return false;
}

void CCsvConverter::ReplaceString(const CString& search, const CString& replace)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    auto& data = m_pReader->GetData();
    for (auto& row : data)
    {
        for (auto& field : row)
        {
            int pos = 0;
            while ((pos = field.Find(search, pos)) != -1)
            {
                field.Replace(search, replace);
                pos += replace.GetLength();
            }
        }
    }
}

void CCsvConverter::ReplaceStringInColumn(int columnIndex, const CString& search, const CString& replace)
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
            int pos = 0;
            while ((pos = row[columnIndex].Find(search, pos)) != -1)
            {
                row[columnIndex].Replace(search, replace);
                pos += replace.GetLength();
            }
        }
    }
}

void CCsvConverter::ReplaceStringInColumn(const CString& columnName, const CString& search, const CString& replace)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        ReplaceStringInColumn(idx, search, replace);
    }
}

void CCsvConverter::SortByColumn(int columnIndex, bool ascending)
{
    if (!m_pReader || !m_pReader->IsLoaded())
        return;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pReader->GetColumnCount())
        return;
    
    auto& data = m_pReader->GetData();
    
    std::sort(data.begin(), data.end(),
        [this, columnIndex, ascending](const std::vector<CString>& a, const std::vector<CString>& b)
        {
            CString valA = (static_cast<size_t>(columnIndex) < a.size()) ? a[columnIndex] : _T("");
            CString valB = (static_cast<size_t>(columnIndex) < b.size()) ? b[columnIndex] : _T("");
            
            int cmp = CompareValues(valA, valB);
            if (ascending)
            {
                return cmp < 0;
            }
            else
            {
                return cmp > 0;
            }
        }
    );
}

void CCsvConverter::SortByColumn(const CString& columnName, bool ascending)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        SortByColumn(idx, ascending);
    }
}
