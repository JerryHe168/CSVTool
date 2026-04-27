#ifndef CSV_PROCESSOR_H
#define CSV_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <ctime>
#include <stdexcept>

namespace csvtool {

struct ValidationError {
    int row;
    int column;
    std::string message;
    
    ValidationError(int r, int c, const std::string& m) 
        : row(r), column(c), message(m) {}
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
    
    // 排序
    void sortByColumn(int columnIndex, bool ascending = true);
    void sortByColumn(const std::string& columnName, bool ascending = true);
    
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

private:
    std::vector<std::string> m_header;
    std::vector<std::vector<std::string>> m_data;
    char m_delimiter;
    
    // 验证规则
    std::vector<std::string> m_requiredColumns;
    std::map<std::string, std::string> m_columnTypes; // "int", "double", "date", "string"
    
    // 辅助方法
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
