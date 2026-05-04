#include "stdafx.h"
#include "CSVModel.h"

CCSVModel::CCSVModel()
{
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
