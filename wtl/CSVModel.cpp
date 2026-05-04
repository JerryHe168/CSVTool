#include "stdafx.h"
#include "CSVModel.h"

CCSVModel::CCSVModel()
{
    m_cleaner.SetReader(&m_reader);
}

CString CCSVModel::GetHeader(size_t col) const
{
    return m_reader.GetHeader(col);
}

CString CCSVModel::GetCell(size_t row, size_t col) const
{
    return m_reader.GetCell(row, col);
}

void CCSVModel::GetStatistics(std::vector<std::pair<CString, CString>>& stats) const
{
    stats.clear();

    if (!m_reader.IsLoaded())
        return;

    CString strTemp;

    strTemp.Format(_T("%d"), (int)m_reader.GetRowCount());
    stats.push_back(std::make_pair(CString(_T("总行数")), strTemp));

    strTemp.Format(_T("%d"), (int)m_reader.GetColumnCount());
    stats.push_back(std::make_pair(CString(_T("总列数")), strTemp));

    size_t nonEmptyCells = 0;
    size_t totalCells = 0;
    const auto& data = m_reader.GetData();
    const auto& headers = m_reader.GetHeaders();

    for (size_t row = 0; row < data.size(); ++row)
    {
        for (size_t col = 0; col < data[row].size(); ++col)
        {
            totalCells++;
            if (!data[row][col].IsEmpty())
                nonEmptyCells++;
        }
    }

    strTemp.Format(_T("%d"), (int)totalCells);
    stats.push_back(std::make_pair(CString(_T("总单元格数")), strTemp));

    strTemp.Format(_T("%d"), (int)nonEmptyCells);
    stats.push_back(std::make_pair(CString(_T("非空单元格数")), strTemp));

    strTemp.Format(_T("%d"), (int)(totalCells - nonEmptyCells));
    stats.push_back(std::make_pair(CString(_T("空单元格数")), strTemp));

    auto allStats = GetAllColumnStatistics();
    for (const auto& pair : allStats)
    {
        const CString& colName = pair.first;
        const CColumnStatistics& colStats = pair.second;
        
        strTemp.Format(_T("列[%s]非空数"), colName);
        CString strValue;
        strValue.Format(_T("%d"), (int)colStats.nonEmptyCount);
        stats.push_back(std::make_pair(strTemp, strValue));
        
        if (colStats.hasValidData)
        {
            strTemp.Format(_T("列[%s]数值统计"), colName);
            CString strNumStats;
            strNumStats.Format(_T("有效数:%d, 最小:%.2f, 最大:%.2f, 平均:%.2f"),
                (int)colStats.validNumericRows,
                colStats.minValue, colStats.maxValue, colStats.average);
            stats.push_back(std::make_pair(strTemp, strNumStats));
        }
        
        if (colStats.hasTextData && !colStats.mode.IsEmpty())
        {
            strTemp.Format(_T("列[%s]众数"), colName);
            CString strModeStats;
            strModeStats.Format(_T("'%s' (出现%d次)"), colStats.mode, (int)colStats.modeCount);
            stats.push_back(std::make_pair(strTemp, strModeStats));
        }
    }
}

bool CCSVModel::LoadFromFile(LPCTSTR pszFilePath)
{
    bool result = m_reader.Read(pszFilePath);
    if (result)
    {
        m_currentFilePath = pszFilePath;
        m_statistics.SetData(&m_reader.GetHeaders(), &m_reader.GetData());
    }
    return result;
}

bool CCSVModel::SaveToFile(LPCTSTR pszFilePath)
{
    if (!m_reader.IsLoaded())
        return false;
    
    bool result = m_writer.Write(pszFilePath, 
                                   m_reader.GetHeaders(), 
                                   m_reader.GetData());
    if (result)
    {
        m_currentFilePath = pszFilePath;
    }
    return result;
}

bool CCSVModel::SaveToFileUTF8(LPCTSTR pszFilePath)
{
    if (!m_reader.IsLoaded())
        return false;
    
    bool result = m_writer.WriteUTF8(pszFilePath, 
                                       m_reader.GetHeaders(), 
                                       m_reader.GetData());
    if (result)
    {
        m_currentFilePath = pszFilePath;
    }
    return result;
}

CColumnStatistics CCSVModel::GetColumnStatistics(int columnIndex) const
{
    return m_statistics.CalculateStatistics(columnIndex);
}

CColumnStatistics CCSVModel::GetColumnStatistics(const CString& columnName) const
{
    return m_statistics.CalculateStatistics(columnName);
}

std::map<CString, CColumnStatistics> CCSVModel::GetAllColumnStatistics() const
{
    return m_statistics.CalculateAllStatistics();
}

int CCSVModel::GetColumnIndex(const CString& columnName) const
{
    auto it = m_columnIndexCache.find(columnName);
    if (it != m_columnIndexCache.end())
    {
        return it->second;
    }

    const auto& headers = m_reader.GetHeaders();
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (headers[i] == columnName)
        {
            m_columnIndexCache[columnName] = static_cast<int>(i);
            return static_cast<int>(i);
        }
    }

    m_columnIndexCache[columnName] = -1;
    return -1;
}

CString CCSVModel::GetColumnName(int columnIndex) const
{
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_reader.GetColumnCount())
        return _T("");
    return m_reader.GetHeaders()[columnIndex];
}

void CCSVModel::InvalidateColumnIndexCache() const
{
    m_columnIndexCache.clear();
}

void CCSVModel::UpdateStatistics()
{
    if (m_reader.IsLoaded())
    {
        m_statistics.SetData(&m_reader.GetHeaders(), &m_reader.GetData());
    }
}

void CCSVModel::SetCell(size_t row, size_t col, const CString& value)
{
    m_reader.SetCell(row, col, value);
}

CString CCSVModel::TrimLeft(const CString& s) const
{
    return m_cleaner.TrimLeft(s);
}

CString CCSVModel::TrimRight(const CString& s) const
{
    return m_cleaner.TrimRight(s);
}

CString CCSVModel::Trim(const CString& s) const
{
    return m_cleaner.Trim(s);
}

bool CCSVModel::IsMissingValue(const CString& value) const
{
    return m_cleaner.IsMissingValue(value);
}

int CCSVModel::CompareValues(const CString& a, const CString& b) const
{
    try
    {
        double numA = _ttof(a);
        double numB = _ttof(b);
        
        CString strA, strB;
        strA.Format(_T("%f"), numA);
        strB.Format(_T("%f"), numB);
        
        CString aTrimmed = a;
        aTrimmed.Trim();
        CString bTrimmed = b;
        bTrimmed.Trim();
        strA.Trim();
        strB.Trim();
        
        if (strA == aTrimmed && strB == bTrimmed)
        {
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

void CCSVModel::TrimWhitespace()
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.TrimWhitespace();
    UpdateStatistics();
}

void CCSVModel::TrimWhitespaceInColumn(int columnIndex)
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.TrimWhitespaceInColumn(columnIndex);
    UpdateStatistics();
}

void CCSVModel::TrimWhitespaceInColumn(const CString& columnName)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        TrimWhitespaceInColumn(idx);
    }
}

void CCSVModel::RemoveRowsWithMissingValues()
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.RemoveRowsWithMissingValues();
    UpdateStatistics();
}

void CCSVModel::RemoveRowsWithMissingValuesInColumn(int columnIndex)
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.RemoveRowsWithMissingValuesInColumn(columnIndex);
    UpdateStatistics();
}

void CCSVModel::RemoveRowsWithMissingValuesInColumn(const CString& columnName)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        RemoveRowsWithMissingValuesInColumn(idx);
    }
}

void CCSVModel::RemoveDuplicateRows()
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.RemoveDuplicateRows();
    UpdateStatistics();
}

void CCSVModel::StandardizeDateFormats(const CString& targetFormat)
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.StandardizeDateFormats(targetFormat);
    UpdateStatistics();
}

void CCSVModel::StandardizeDateFormatsInColumn(int columnIndex, const CString& targetFormat)
{
    if (!m_reader.IsLoaded())
        return;
    m_cleaner.StandardizeDateFormatsInColumn(columnIndex, targetFormat);
    UpdateStatistics();
}

void CCSVModel::StandardizeDateFormatsInColumn(const CString& columnName, const CString& targetFormat)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        StandardizeDateFormatsInColumn(idx, targetFormat);
    }
}

bool CCSVModel::AddColumn(int position, const CString& columnName, const CString& defaultValue)
{
    if (!m_reader.IsLoaded())
        return false;
    if (position < 0 || static_cast<size_t>(position) > m_reader.GetColumnCount())
        return false;

    auto& headers = m_reader.GetHeaders();
    auto& data = m_reader.GetData();

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

    InvalidateColumnIndexCache();
    UpdateStatistics();
    return true;
}

bool CCSVModel::AddColumn(const CString& columnName, const CString& defaultValue)
{
    return AddColumn(static_cast<int>(m_reader.GetColumnCount()), columnName, defaultValue);
}

bool CCSVModel::RemoveColumn(int columnIndex)
{
    if (!m_reader.IsLoaded())
        return false;
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_reader.GetColumnCount())
        return false;

    auto& headers = m_reader.GetHeaders();
    auto& data = m_reader.GetData();

    headers.erase(headers.begin() + columnIndex);

    for (auto& row : data)
    {
        if (static_cast<size_t>(columnIndex) < row.size())
        {
            row.erase(row.begin() + columnIndex);
        }
    }

    InvalidateColumnIndexCache();
    UpdateStatistics();
    return true;
}

bool CCSVModel::RemoveColumn(const CString& columnName)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        return RemoveColumn(idx);
    }
    return false;
}

bool CCSVModel::RenameColumn(int columnIndex, const CString& newName)
{
    if (!m_reader.IsLoaded())
        return false;
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_reader.GetColumnCount())
        return false;

    auto& headers = m_reader.GetHeaders();
    headers[columnIndex] = newName;

    InvalidateColumnIndexCache();
    return true;
}

bool CCSVModel::RenameColumn(const CString& oldName, const CString& newName)
{
    int idx = GetColumnIndex(oldName);
    if (idx >= 0)
    {
        return RenameColumn(idx, newName);
    }
    return false;
}

void CCSVModel::ReplaceString(const CString& search, const CString& replace)
{
    if (!m_reader.IsLoaded())
        return;

    auto& data = m_reader.GetData();
    for (auto& row : data)
    {
        for (auto& field : row)
        {
            int pos = 0;
            while ((pos = field.Find(search, pos)) != -1)
            {
                field.Delete(pos, search.GetLength());
                field.Insert(pos, replace);
                pos += replace.GetLength();
            }
        }
    }
    UpdateStatistics();
}

void CCSVModel::ReplaceStringInColumn(int columnIndex, const CString& search, const CString& replace)
{
    if (!m_reader.IsLoaded())
        return;
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_reader.GetColumnCount())
        return;

    auto& data = m_reader.GetData();
    for (auto& row : data)
    {
        if (static_cast<size_t>(columnIndex) < row.size())
        {
            int pos = 0;
            while ((pos = row[columnIndex].Find(search, pos)) != -1)
            {
                row[columnIndex].Delete(pos, search.GetLength());
                row[columnIndex].Insert(pos, replace);
                pos += replace.GetLength();
            }
        }
    }
    UpdateStatistics();
}

void CCSVModel::ReplaceStringInColumn(const CString& columnName, const CString& search, const CString& replace)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        ReplaceStringInColumn(idx, search, replace);
    }
}

void CCSVModel::SortByColumn(int columnIndex, bool ascending)
{
    if (!m_reader.IsLoaded())
        return;
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_reader.GetColumnCount())
        return;

    auto& data = m_reader.GetData();
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

void CCSVModel::SortByColumn(const CString& columnName, bool ascending)
{
    int idx = GetColumnIndex(columnName);
    if (idx >= 0)
    {
        SortByColumn(idx, ascending);
    }
}
