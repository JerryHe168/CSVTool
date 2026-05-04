#include "stdafx.h"
#include "CsvStatistics.h"

CCsvStatistics::CCsvStatistics()
    : m_pHeaders(NULL)
    , m_pData(NULL)
{
}

void CCsvStatistics::SetData(const std::vector<CString>* pHeaders, 
                              const std::vector<std::vector<CString>>* pData)
{
    m_pHeaders = pHeaders;
    m_pData = pData;
}

CString CCsvStatistics::Trim(const CString& s)
{
    CString result = s;
    result.Trim();
    return result;
}

bool CCsvStatistics::IsMissingValue(const CString& value)
{
    CString trimmed = Trim(value);
    return trimmed.IsEmpty() || 
           trimmed == _T("NA") || 
           trimmed == _T("N/A") || 
           trimmed == _T("null") || 
           trimmed == _T("NULL");
}

bool CCsvStatistics::TryParseDouble(const CString& str, double& result)
{
    if (str.IsEmpty())
        return false;

    CString trimmed = Trim(str);
    if (trimmed.IsEmpty())
        return false;

    USES_CONVERSION;
    const char* psz = T2CA(trimmed);
    char* endPtr = NULL;
    errno = 0;
    
    result = strtod(psz, &endPtr);
    
    if (errno == ERANGE || (endPtr == psz) || (*endPtr != '\0'))
    {
        return false;
    }
    return true;
}

int CCsvStatistics::GetColumnIndex(const CString& columnName) const
{
    if (!m_pHeaders)
        return -1;

    for (size_t i = 0; i < m_pHeaders->size(); ++i)
    {
        if ((*m_pHeaders)[i] == columnName)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

CString CCsvStatistics::GetColumnName(int columnIndex) const
{
    if (!m_pHeaders || columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pHeaders->size())
        return _T("");
    return (*m_pHeaders)[columnIndex];
}

size_t CCsvStatistics::GetRowCount() const
{
    return m_pData ? m_pData->size() : 0;
}

size_t CCsvStatistics::GetColumnCount() const
{
    return m_pHeaders ? m_pHeaders->size() : 0;
}

CColumnStatistics CCsvStatistics::CalculateStatistics(const CString& columnName) const
{
    int idx = GetColumnIndex(columnName);
    if (idx < 0)
    {
        CColumnStatistics stats;
        stats.columnName = columnName;
        return stats;
    }
    return CalculateStatistics(idx);
}

CColumnStatistics CCsvStatistics::CalculateStatistics(int columnIndex) const
{
    CColumnStatistics stats;
    
    if (!m_pHeaders || !m_pData)
        return stats;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_pHeaders->size())
        return stats;
    
    stats.columnName = (*m_pHeaders)[columnIndex];
    stats.totalRows = m_pData->size();
    
    std::vector<double> numericValues;
    std::map<CString, size_t> valueCounts;
    size_t nonEmptyCount = 0;
    size_t missingCount = 0;
    
    size_t textMinLength = static_cast<size_t>(-1);
    size_t textMaxLength = 0;
    size_t textTotalLength = 0;
    size_t textCount = 0;
    
    double minVal = 0.0;
    double maxVal = 0.0;
    double sumVal = 0.0;
    size_t validCount = 0;
    bool firstValue = true;
    
    double mean = 0.0;
    double M2 = 0.0;
    
    for (const auto& row : *m_pData)
    {
        const CString& value = 
            (static_cast<size_t>(columnIndex) < row.size()) ? row[columnIndex] : _T("");
        
        if (value.IsEmpty() || IsMissingValue(value))
        {
            missingCount++;
            continue;
        }
        
        nonEmptyCount++;
        valueCounts[value]++;
        
        textCount++;
        size_t len = value.GetLength();
        textTotalLength += len;
        if (len < textMinLength) textMinLength = len;
        if (len > textMaxLength) textMaxLength = len;
        
        double num = 0.0;
        if (TryParseDouble(value, num))
        {
            if (firstValue)
            {
                minVal = num;
                maxVal = num;
                firstValue = false;
            }
            else
            {
                if (num < minVal) minVal = num;
                if (num > maxVal) maxVal = num;
            }
            
            sumVal += num;
            validCount++;
            numericValues.push_back(num);
            
            double delta = num - mean;
            mean += delta / static_cast<double>(validCount);
            double delta2 = num - mean;
            M2 += delta * delta2;
        }
    }
    
    stats.nonEmptyCount = nonEmptyCount;
    stats.uniqueCount = valueCounts.size();
    stats.missingCount = missingCount;
    stats.validNumericRows = validCount;
    stats.hasValidData = (validCount > 0);
    
    if (stats.hasValidData)
    {
        stats.minValue = minVal;
        stats.maxValue = maxVal;
        stats.sum = sumVal;
        stats.average = sumVal / static_cast<double>(validCount);
        
        if (numericValues.size() > 0)
        {
            std::sort(numericValues.begin(), numericValues.end());
            
            if (validCount % 2 == 1)
            {
                stats.median = numericValues[validCount / 2];
            }
            else
            {
                size_t mid = validCount / 2;
                if (mid > 0 && mid < numericValues.size())
                {
                    stats.median = (numericValues[mid - 1] + numericValues[mid]) / 2.0;
                }
                else
                {
                    stats.median = 0.0;
                }
            }
        }
        
        if (validCount > 1)
        {
            stats.variance = M2 / static_cast<double>(validCount);
            stats.standardDeviation = std::sqrt(stats.variance);
        }
        else
        {
            stats.variance = 0.0;
            stats.standardDeviation = 0.0;
        }
    }
    
    stats.hasTextData = (textCount > 0);
    if (stats.hasTextData)
    {
        stats.minLength = textMinLength;
        stats.maxLength = textMaxLength;
        stats.avgLength = static_cast<double>(textTotalLength) / static_cast<double>(textCount);
        
        size_t maxCount = 0;
        CString modeValue = _T("");
        for (const auto& pair : valueCounts)
        {
            if (pair.second > maxCount)
            {
                maxCount = pair.second;
                modeValue = pair.first;
            }
        }
        stats.mode = modeValue;
        stats.modeCount = maxCount;
    }
    
    return stats;
}

std::map<CString, CColumnStatistics> CCsvStatistics::CalculateAllStatistics() const
{
    std::map<CString, CColumnStatistics> result;
    
    if (!m_pHeaders)
        return result;
    
    for (size_t i = 0; i < m_pHeaders->size(); ++i)
    {
        result[(*m_pHeaders)[i]] = CalculateStatistics(static_cast<int>(i));
    }
    
    return result;
}
