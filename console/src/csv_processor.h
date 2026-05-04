#ifndef CSV_PROCESSOR_H
#define CSV_PROCESSOR_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <ctime>
#include <stdexcept>
#include <filesystem>

namespace csvtool {

struct ValidationError {
    int row;
    int column;
    std::string message;
    
    ValidationError(int r, int c, const std::string& m) 
        : row(r), column(c), message(m) {}
};

struct ColumnStatistics {
    std::string columnName;
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
    
    std::string mode;
    size_t modeCount;
    size_t minLength;
    size_t maxLength;
    double avgLength;
    bool hasTextData;
    
    ColumnStatistics() 
        : totalRows(0), nonEmptyCount(0), uniqueCount(0), missingCount(0),
          validNumericRows(0), minValue(0.0), maxValue(0.0), sum(0.0), 
          average(0.0), median(0.0), standardDeviation(0.0), variance(0.0), 
          hasValidData(false),
          mode(""), modeCount(0), minLength(0), maxLength(0), 
          avgLength(0.0), hasTextData(false) {}
};

enum class FilterOperator {
    EQUALS,
    NOT_EQUALS,
    GREATER_THAN,
    LESS_THAN,
    GREATER_OR_EQUAL,
    LESS_OR_EQUAL,
    CONTAINS,
    NOT_CONTAINS,
    STARTS_WITH,
    ENDS_WITH,
    REGEX_MATCH
};

enum class ExportFormat {
    CSV,
    JSON,
    XML,
    MARKDOWN,
    HTML,
    SQL
};

struct FilterCondition {
    std::string columnName;
    FilterOperator op;
    std::string value;
    bool caseSensitive;
    
    FilterCondition() 
        : columnName(""), op(FilterOperator::EQUALS), value(""), caseSensitive(true) {}
};

class CSVProcessor {
public:
    CSVProcessor();
    ~CSVProcessor() = default;
    
    // 读取和写入
    bool read(const std::string& filename, char delimiter = ',');
    bool write(const std::string& filename, char delimiter = ',') const;
    
    // 正确性验证
    bool validate() const;
    std::vector<ValidationError> getValidationErrors() const;
    
    // 数据清洗
    void trimWhitespace();
    void trimWhitespaceInColumn(int columnIndex);
    void trimWhitespaceInColumn(const std::string& columnName);
    
    void removeRowsWithMissingValues();
    void removeRowsWithMissingValuesInColumn(int columnIndex);
    void removeRowsWithMissingValuesInColumn(const std::string& columnName);
    
    void removeDuplicateRows();
    
    void standardizeDateFormats(const std::string& targetFormat = "%Y-%m-%d");
    void standardizeDateFormatsInColumn(int columnIndex, const std::string& targetFormat = "%Y-%m-%d");
    void standardizeDateFormatsInColumn(const std::string& columnName, const std::string& targetFormat = "%Y-%m-%d");
    
    // 数据转换
    bool addColumn(int position, const std::string& columnName, const std::string& defaultValue = "");
    bool addColumn(const std::string& columnName, const std::string& defaultValue = "");
    
    bool removeColumn(int columnIndex);
    bool removeColumn(const std::string& columnName);
    
    bool renameColumn(int columnIndex, const std::string& newName);
    bool renameColumn(const std::string& oldName, const std::string& newName);
    
    void replaceString(const std::string& search, const std::string& replace);
    void replaceStringInColumn(int columnIndex, const std::string& search, const std::string& replace);
    void replaceStringInColumn(const std::string& columnName, const std::string& search, const std::string& replace);
    
    // 计算列
    bool addCalculatedColumn(const std::string& newColumnName, const std::string& expression);
    bool addCalculatedColumn(int position, const std::string& newColumnName, const std::string& expression);
    
    // 统计功能
    ColumnStatistics calculateStatistics(const std::string& columnName) const;
    ColumnStatistics calculateStatistics(int columnIndex) const;
    std::map<std::string, ColumnStatistics> calculateAllStatistics() const;
    void printStatistics(const std::string& columnName) const;
    void printAllStatistics() const;
    
    // 排序
    void sortByColumn(int columnIndex, bool ascending = true);
    void sortByColumn(const std::string& columnName, bool ascending = true);
    
    // 数据筛选
    void filter(const FilterCondition& condition);
    void filter(const std::vector<FilterCondition>& conditions, bool matchAll = true);
    void filterByRegex(const std::string& columnName, const std::string& pattern, bool caseSensitive = true);
    void filterByRegex(const std::string& pattern, bool caseSensitive = true);
    
    // 数据导出
    bool exportToFile(const std::string& filename, ExportFormat format, const std::string& tableName = "") const;
    bool exportToFile(const std::string& filename, ExportFormat format, const std::vector<std::string>& columns, const std::string& tableName = "") const;
    
    std::string toCSV() const;
    std::string toJSON() const;
    std::string toXML() const;
    std::string toMarkdown() const;
    std::string toHTML() const;
    std::string toSQL(const std::string& tableName = "data") const;
    
    // 辅助方法
    int getColumnIndex(const std::string& columnName) const;
    std::string getColumnName(int columnIndex) const;
    size_t getRowCount() const;
    size_t getColumnCount() const;
    void printInfo() const;
    
    // 数据访问
    const std::vector<std::string>& getHeader() const { return m_header; }
    const std::vector<std::vector<std::string>>& getData() const { return m_data; }
    
    // 设置验证规则
    void setRequiredColumns(const std::vector<std::string>& columns);
    void setColumnTypes(const std::map<std::string, std::string>& types);
    void clearValidationRules();
    
    // 合并功能
    bool mergeFrom(const CSVProcessor& other, bool addSourceColumn = false, const std::string& sourceValue = "");
    bool mergeColumnsFrom(const CSVProcessor& other);
    
    // 批量处理辅助
    static std::vector<std::string> findCSVFiles(const std::string& directory, bool recursive = false);

private:
    std::vector<std::string> m_header;
    std::vector<std::vector<std::string>> m_data;
    char m_delimiter;
    
    // 验证规则
    std::vector<std::string> m_requiredColumns;
    std::map<std::string, std::string> m_columnTypes; // "int", "double", "date", "string"
    
    // 列名索引缓存（mutable 允许在 const 方法中修改）
    mutable std::unordered_map<std::string, int> m_columnIndexCache;
    void invalidateColumnIndexCache() const { m_columnIndexCache.clear(); }
    
    // 辅助方法
    std::vector<ValidationError> validateInternal() const;
    std::vector<std::string> parseCSVLine(const std::string& line);
    std::string formatCSVField(const std::string& field) const;
    
    std::string trim(const std::string& s) const;
    std::string trimLeft(const std::string& s) const;
    std::string trimRight(const std::string& s) const;
    
    bool isMissingValue(const std::string& value) const;
    
    std::string parseAndFormatDate(const std::string& dateStr, const std::string& targetFormat) const;
    bool tryParseDate(const std::string& dateStr, std::tm& tm) const;
    
    int compareValues(const std::string& a, const std::string& b) const;
};

} // namespace csvtool

#endif // CSV_PROCESSOR_H
