#pragma once
#include "stdafx.h"

struct CColumnStatistics
{
    CString columnName;
    size_t totalRows;
    size_t nonEmptyCount;
    size_t uniqueCount;
    size_t missingCount;
    
    size_t validNumericRows;
    double minValue;
    double maxValue;
    double sum;
    double average;
    double median;
    double standardDeviation;
    double variance;
    bool hasValidData;
    
    CString mode;
    size_t modeCount;
    size_t minLength;
    size_t maxLength;
    double avgLength;
    bool hasTextData;
    
    CColumnStatistics()
        : totalRows(0), nonEmptyCount(0), uniqueCount(0), missingCount(0),
          validNumericRows(0), minValue(0.0), maxValue(0.0), sum(0.0), 
          average(0.0), median(0.0), standardDeviation(0.0), variance(0.0), 
          hasValidData(false),
          mode(_T("")), modeCount(0), minLength(0), maxLength(0), 
          avgLength(0.0), hasTextData(false) {}
};

class CCsvStatistics
{
protected:
    const std::vector<CString>* m_pHeaders;
    const std::vector<std::vector<CString>>* m_pData;

public:
    CCsvStatistics();
    virtual ~CCsvStatistics() {}

    void SetData(const std::vector<CString>* pHeaders, 
                 const std::vector<std::vector<CString>>* pData);
    
    CColumnStatistics CalculateStatistics(int columnIndex) const;
    CColumnStatistics CalculateStatistics(const CString& columnName) const;
    std::map<CString, CColumnStatistics> CalculateAllStatistics() const;
    
    int GetColumnIndex(const CString& columnName) const;
    CString GetColumnName(int columnIndex) const;
    size_t GetRowCount() const;
    size_t GetColumnCount() const;

protected:
    static bool TryParseDouble(const CString& str, double& result);
    static bool IsMissingValue(const CString& value);
    static CString Trim(const CString& s);
};
