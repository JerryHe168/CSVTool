#include "csv_processor.h"
#include <iostream>
#include <cctype>
#include <set>
#include <unordered_set>
#include <codecvt>

namespace csvtool {

CSVProcessor::CSVProcessor() : m_delimiter(','), m_header{}, m_data{} {}

// 读取CSV文件
bool CSVProcessor::read(const std::string& filename, char delimiter) {
    m_delimiter = delimiter;
    m_header.clear();
    m_data.clear();
    invalidateColumnIndexCache();
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    
    // 检查并跳过BOM (UTF-8 BOM: 0xEF 0xBB 0xBF)
    char bom[3] = {0};
    if (file.read(bom, 3)) {
        if (!(static_cast<unsigned char>(bom[0]) == 0xEF && 
              static_cast<unsigned char>(bom[1]) == 0xBB && 
              static_cast<unsigned char>(bom[2]) == 0xBF)) {
            // 不是BOM，将字符放回
            file.seekg(0);
        }
    } else {
        // 文件太小，重置到开头
        file.clear();
        file.seekg(0);
    }
    
    // 读取表头
    if (std::getline(file, line)) {
        // 处理可能的\r\n换行符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        m_header = parseCSVLine(line);
    } else {
        std::cerr << "Error: File is empty" << std::endl;
        file.close();
        return false;
    }
    
    // 读取数据
    while (std::getline(file, line)) {
        // 处理可能的\r\n换行符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) continue;
        
        std::vector<std::string> row = parseCSVLine(line);
        
        // 检查列数是否匹配
        if (row.size() != m_header.size()) {
            std::cerr << "Warning: Row " << m_data.size() + 1 
                      << " has " << row.size() << " columns, expected " 
                      << m_header.size() << std::endl;
        }
        
        m_data.push_back(row);
    }
    
    file.close();
    return true;
}

// 写入CSV文件
bool CSVProcessor::write(const std::string& filename, char delimiter) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return false;
    }
    
    // 写入表头
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (i > 0) file << delimiter;
        file << formatCSVField(m_header[i]);
    }
    file << '\n';
    
    // 写入数据
    for (const auto& row : m_data) {
        for (size_t i = 0; i < m_header.size(); ++i) {
            if (i > 0) file << delimiter;
            if (i < row.size()) {
                file << formatCSVField(row[i]);
            }
        }
        file << '\n';
    }
    
    file.close();
    return true;
}

// 解析CSV行
std::vector<std::string> CSVProcessor::parseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                // 双引号转义
                current += '"';
                ++i;
            } else {
                // 切换引号状态
                inQuotes = !inQuotes;
            }
        } else if (c == m_delimiter && !inQuotes) {
            // 字段结束
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    
    // 添加最后一个字段
    result.push_back(current);
    return result;
}

// 格式化CSV字段
std::string CSVProcessor::formatCSVField(const std::string& field) const {
    // 检查是否需要引号
    bool needsQuotes = false;
    for (char c : field) {
        if (c == m_delimiter || c == '"' || c == '\n' || c == '\r') {
            needsQuotes = true;
            break;
        }
    }
    
    if (!needsQuotes) {
        return field;
    }
    
    // 添加引号并转义内部引号
    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

// 正确性验证
std::vector<ValidationError> CSVProcessor::validateInternal() const {
    std::vector<ValidationError> errors;
    
    // 检查必需列
    for (const std::string& col : m_requiredColumns) {
        bool found = false;
        for (const std::string& h : m_header) {
            if (h == col) {
                found = true;
                break;
            }
        }
        if (!found) {
            errors.emplace_back(0, -1, "Required column missing: " + col);
        }
    }
    
    // 检查每一行的列数
    for (size_t rowIdx = 0; rowIdx < m_data.size(); ++rowIdx) {
        const auto& row = m_data[rowIdx];
        if (row.size() != m_header.size()) {
            errors.emplace_back(
                static_cast<int>(rowIdx + 1), 
                static_cast<int>(row.size()),
                "Column count mismatch: expected " + std::to_string(m_header.size()) + 
                ", got " + std::to_string(row.size())
            );
        }
        
        // 检查列类型
        for (size_t colIdx = 0; colIdx < row.size() && colIdx < m_header.size(); ++colIdx) {
            const std::string& colName = m_header[colIdx];
            const std::string& value = row[colIdx];
            
            if (m_columnTypes.find(colName) != m_columnTypes.end()) {
                const std::string& type = m_columnTypes.at(colName);
                
                if (type == "int" && !value.empty()) {
                    try {
                        size_t pos;
                        std::stoll(value, &pos);
                        if (pos != value.size()) {
                            throw std::invalid_argument("");
                        }
                    } catch (...) {
                        errors.emplace_back(
                            static_cast<int>(rowIdx + 1), 
                            static_cast<int>(colIdx),
                            "Invalid integer value: '" + value + "'"
                        );
                    }
                } else if (type == "double" && !value.empty()) {
                    try {
                        size_t pos;
                        std::stod(value, &pos);
                        if (pos != value.size()) {
                            throw std::invalid_argument("");
                        }
                    } catch (...) {
                        errors.emplace_back(
                            static_cast<int>(rowIdx + 1), 
                            static_cast<int>(colIdx),
                            "Invalid numeric value: '" + value + "'"
                        );
                    }
                } else if (type == "date" && !value.empty()) {
                    std::tm tm = {};
                    if (!tryParseDate(value, tm)) {
                        errors.emplace_back(
                            static_cast<int>(rowIdx + 1), 
                            static_cast<int>(colIdx),
                            "Invalid date value: '" + value + "'"
                        );
                    }
                }
            }
        }
    }
    
    return errors;
}

bool CSVProcessor::validate() const {
    return validateInternal().empty();
}

std::vector<ValidationError> CSVProcessor::getValidationErrors() const {
    return validateInternal();
}

// 数据清洗 - 去除空白
std::string CSVProcessor::trim(const std::string& s) const {
    return trimRight(trimLeft(s));
}

std::string CSVProcessor::trimLeft(const std::string& s) const {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    return s.substr(start);
}

std::string CSVProcessor::trimRight(const std::string& s) const {
    size_t end = s.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(0, end);
}

void CSVProcessor::trimWhitespace() {
    for (auto& row : m_data) {
        for (auto& field : row) {
            field = trim(field);
        }
    }
}

void CSVProcessor::trimWhitespaceInColumn(int columnIndex) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return;
    }
    
    for (auto& row : m_data) {
        if (static_cast<size_t>(columnIndex) < row.size()) {
            row[columnIndex] = trim(row[columnIndex]);
        }
    }
}

void CSVProcessor::trimWhitespaceInColumn(const std::string& columnName) {
    int idx = getColumnIndex(columnName);
    if (idx >= 0) {
        trimWhitespaceInColumn(idx);
    }
}

// 数据清洗 - 删除缺失值
bool CSVProcessor::isMissingValue(const std::string& value) const {
    std::string trimmed = trim(value);
    return trimmed.empty() || 
           trimmed == "NA" || 
           trimmed == "N/A" || 
           trimmed == "null" || 
           trimmed == "NULL";
}

void CSVProcessor::removeRowsWithMissingValues() {
    std::vector<std::vector<std::string>> newData;
    
    for (const auto& row : m_data) {
        bool hasMissing = false;
        for (const auto& field : row) {
            if (isMissingValue(field)) {
                hasMissing = true;
                break;
            }
        }
        if (!hasMissing) {
            newData.push_back(row);
        }
    }
    
    m_data = newData;
}

void CSVProcessor::removeRowsWithMissingValuesInColumn(int columnIndex) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return;
    }
    
    std::vector<std::vector<std::string>> newData;
    
    for (const auto& row : m_data) {
        if (static_cast<size_t>(columnIndex) < row.size()) {
            if (!isMissingValue(row[columnIndex])) {
                newData.push_back(row);
            }
        } else {
            // 如果列不存在，则视为缺失
        }
    }
    
    m_data = newData;
}

void CSVProcessor::removeRowsWithMissingValuesInColumn(const std::string& columnName) {
    int idx = getColumnIndex(columnName);
    if (idx >= 0) {
        removeRowsWithMissingValuesInColumn(idx);
    }
}

// 数据清洗 - 删除重复行
void CSVProcessor::removeDuplicateRows() {
    std::vector<std::vector<std::string>> newData;
    std::set<std::string> seen;
    
    for (const auto& row : m_data) {
        // 创建行的唯一标识
        std::string key;
        for (const auto& field : row) {
            key += field + "\x1F"; // 使用单元分隔符
        }
        
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            newData.push_back(row);
        }
    }
    
    m_data = newData;
}

namespace {

// 前向声明
int monthNameToNumber(const std::string& name);

// 辅助函数：检查字符串是否只包含数字
bool isAllDigits(const std::string& s) {
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

// 辅助函数：检查字符是否是日期分隔符
bool isDateSeparator(char c) {
    return c == '-' || c == '/' || c == '.';
}

// 辅助函数：跳过空白字符
void skipWhitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
        pos++;
    }
}

// 辅助函数：解析整数
bool parseInt(const std::string& s, size_t& pos, int& result, int minDigits = 1, int maxDigits = 10) {
    size_t start = pos;
    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
        pos++;
    }
    if (pos - start < (size_t)minDigits || pos - start > (size_t)maxDigits) {
        return false;
    }
    try {
        result = std::stoi(s.substr(start, pos - start));
        return true;
    } catch (...) {
        return false;
    }
}

// 辅助函数：解析月份名称
int parseMonthName(const std::string& s, size_t& pos) {
    size_t start = pos;
    while (pos < s.size() && std::isalpha(static_cast<unsigned char>(s[pos]))) {
        pos++;
    }
    if (pos == start) {
        return -1;
    }
    std::string monthName = s.substr(start, pos - start);
    return monthNameToNumber(monthName);
}

// 辅助函数：月份名称到数字的映射
int monthNameToNumber(const std::string& name) {
    static const std::map<std::string, int> monthMap = {
        {"jan", 1}, {"january", 1},
        {"feb", 2}, {"february", 2},
        {"mar", 3}, {"march", 3},
        {"apr", 4}, {"april", 4},
        {"may", 5},
        {"jun", 6}, {"june", 6},
        {"jul", 7}, {"july", 7},
        {"aug", 8}, {"august", 8},
        {"sep", 9}, {"september", 9},
        {"oct", 10}, {"october", 10},
        {"nov", 11}, {"november", 11},
        {"dec", 12}, {"december", 12}
    };
    
    std::string lower;
    for (char c : name) {
        lower += std::tolower(static_cast<unsigned char>(c));
    }
    
    auto it = monthMap.find(lower);
    if (it != monthMap.end()) {
        return it->second;
    }
    return -1;
}

// 辅助函数：将日期组件格式化为YYYY-MM-DD
std::string formatDateISO(int year, int month, int day) {
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(4) << year << "-"
       << std::setfill('0') << std::setw(2) << month << "-"
       << std::setfill('0') << std::setw(2) << day;
    return ss.str();
}

// 辅助函数：检查年份是否合理
bool isValidYear(int year) {
    return year >= 1900 && year <= 2100;
}

// 辅助函数：检查月份是否合理
bool isValidMonth(int month) {
    return month >= 1 && month <= 12;
}

// 辅助函数：检查日期是否合理（简单检查，不考虑闰年）
bool isValidDay(int year, int month, int day) {
    if (day < 1 || day > 31) return false;
    
    static const int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month == 2) {
        // 闰年检查
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return day <= (isLeap ? 29 : 28);
    }
    
    return day <= daysInMonth[month];
}

} // 匿名命名空间

// 数据清洗 - 标准化日期格式
bool CSVProcessor::tryParseDate(const std::string& dateStr, std::tm& tm) const {
    std::string trimmed = trim(dateStr);
    
    if (trimmed.empty()) {
        return false;
    }
    
    bool hasDigit = false;
    for (char c : trimmed) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            hasDigit = true;
            break;
        }
    }
    if (!hasDigit) {
        return false;
    }
    
    int year = 0, month = 0, day = 0;
    size_t pos = 0;
    
    // 格式3: YYYYMMDD (8位数字)
    if (trimmed.size() == 8 && isAllDigits(trimmed)) {
        year = std::stoi(trimmed.substr(0, 4));
        month = std::stoi(trimmed.substr(4, 2));
        day = std::stoi(trimmed.substr(6, 2));
        
        if (isValidYear(year) && isValidMonth(month) && isValidDay(year, month, day)) {
            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            return true;
        }
        return false;
    }
    
    // 检查是否是带时间的日期 (格式6)
    // YYYY-MM-DD HH:MM:SS 或 YYYY-MM-DDTHH:MM:SS
    {
        size_t spacePos = trimmed.find(' ');
        size_t tPos = trimmed.find('T');
        size_t timePos = std::string::npos;
        
        if (spacePos != std::string::npos) {
            timePos = spacePos;
        } else if (tPos != std::string::npos) {
            timePos = tPos;
        }
        
        if (timePos != std::string::npos && timePos >= 8) {
            std::string datePart = trimmed.substr(0, timePos);
            pos = 0;
            
            if (parseInt(datePart, pos, year, 4, 4)) {
                if (pos < datePart.size() && isDateSeparator(datePart[pos])) {
                    pos++;
                    if (parseInt(datePart, pos, month, 1, 2)) {
                        if (pos < datePart.size() && isDateSeparator(datePart[pos])) {
                            pos++;
                            if (parseInt(datePart, pos, day, 1, 2) && pos == datePart.size()) {
                                if (isValidYear(year) && isValidMonth(month) && isValidDay(year, month, day)) {
                                    tm.tm_year = year - 1900;
                                    tm.tm_mon = month - 1;
                                    tm.tm_mday = day;
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 格式1: YYYY-MM-DD / YYYY/MM/DD / YYYY.MM.DD
    // 格式2: DD-MM-YYYY / DD/MM/YYYY / DD.MM.YYYY
    // 同时尝试两种格式，通过年份范围判断
    {
        pos = 0;
        int num1 = 0, num2 = 0, num3 = 0;
        
        if (parseInt(trimmed, pos, num1, 1, 4)) {
            if (pos < trimmed.size() && isDateSeparator(trimmed[pos])) {
                pos++;
                if (parseInt(trimmed, pos, num2, 1, 2)) {
                    if (pos < trimmed.size() && isDateSeparator(trimmed[pos])) {
                        pos++;
                        if (parseInt(trimmed, pos, num3, 1, 4) && pos == trimmed.size()) {
                            // 两种可能的格式
                            // 格式1: YYYY-MM-DD (num1=年, num2=月, num3=日)
                            if (isValidYear(num1) && isValidMonth(num2) && isValidDay(num1, num2, num3)) {
                                year = num1;
                                month = num2;
                                day = num3;
                                tm.tm_year = year - 1900;
                                tm.tm_mon = month - 1;
                                tm.tm_mday = day;
                                return true;
                            }
                            // 格式2: DD-MM-YYYY (num1=日, num2=月, num3=年)
                            if (isValidYear(num3) && isValidMonth(num2) && isValidDay(num3, num2, num1)) {
                                year = num3;
                                month = num2;
                                day = num1;
                                tm.tm_year = year - 1900;
                                tm.tm_mon = month - 1;
                                tm.tm_mday = day;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 格式4: "Jan 5, 2023" / "January 5, 2023"
    pos = 0;
    skipWhitespace(trimmed, pos);
    {
        int m = parseMonthName(trimmed, pos);
        if (m > 0) {
            skipWhitespace(trimmed, pos);
            if (parseInt(trimmed, pos, day, 1, 2)) {
                if (pos < trimmed.size() && trimmed[pos] == ',') {
                    pos++;
                    skipWhitespace(trimmed, pos);
                    if (parseInt(trimmed, pos, year, 4, 4) && pos == trimmed.size()) {
                        if (isValidYear(year) && isValidDay(year, m, day)) {
                            tm.tm_year = year - 1900;
                            tm.tm_mon = m - 1;
                            tm.tm_mday = day;
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    // 格式5: "5 Jan 2023" / "5 January 2023"
    pos = 0;
    skipWhitespace(trimmed, pos);
    if (parseInt(trimmed, pos, day, 1, 2)) {
        skipWhitespace(trimmed, pos);
        int m = parseMonthName(trimmed, pos);
        if (m > 0) {
            skipWhitespace(trimmed, pos);
            if (parseInt(trimmed, pos, year, 4, 4) && pos == trimmed.size()) {
                if (isValidYear(year) && isValidDay(year, m, day)) {
                    tm.tm_year = year - 1900;
                    tm.tm_mon = m - 1;
                    tm.tm_mday = day;
                    return true;
                }
            }
        }
    }
    
    return false;
}

std::string CSVProcessor::parseAndFormatDate(const std::string& dateStr, const std::string& targetFormat) const {
    std::tm tm = {};
    if (tryParseDate(dateStr, tm)) {
        // 目前只支持YYYY-MM-DD格式
        int year = tm.tm_year + 1900;
        int month = tm.tm_mon + 1;
        int day = tm.tm_mday;
        return formatDateISO(year, month, day);
    }
    return dateStr; // 如果无法解析，返回原始值
}

void CSVProcessor::standardizeDateFormats(const std::string& targetFormat) {
    for (auto& row : m_data) {
        for (auto& field : row) {
            std::tm tm = {};
            if (tryParseDate(field, tm)) {
                int year = tm.tm_year + 1900;
                int month = tm.tm_mon + 1;
                int day = tm.tm_mday;
                field = formatDateISO(year, month, day);
            }
        }
    }
}

void CSVProcessor::standardizeDateFormatsInColumn(int columnIndex, const std::string& targetFormat) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return;
    }
    
    for (auto& row : m_data) {
        if (static_cast<size_t>(columnIndex) < row.size()) {
            row[columnIndex] = parseAndFormatDate(row[columnIndex], targetFormat);
        }
    }
}

void CSVProcessor::standardizeDateFormatsInColumn(const std::string& columnName, const std::string& targetFormat) {
    int idx = getColumnIndex(columnName);
    if (idx >= 0) {
        standardizeDateFormatsInColumn(idx, targetFormat);
    }
}

// 数据转换 - 添加列
bool CSVProcessor::addColumn(int position, const std::string& columnName, const std::string& defaultValue) {
    if (position < 0 || static_cast<size_t>(position) > m_header.size()) {
        return false;
    }
    
    // 插入表头
    m_header.insert(m_header.begin() + position, columnName);
    
    // 插入数据
    for (auto& row : m_data) {
        if (static_cast<size_t>(position) <= row.size()) {
            row.insert(row.begin() + position, defaultValue);
        } else {
            // 如果行太短，填充到指定位置
            while (row.size() < static_cast<size_t>(position)) {
                row.push_back("");
            }
            row.push_back(defaultValue);
        }
    }
    
    invalidateColumnIndexCache();
    return true;
}

bool CSVProcessor::addColumn(const std::string& columnName, const std::string& defaultValue) {
    return addColumn(static_cast<int>(m_header.size()), columnName, defaultValue);
}

// 数据转换 - 删除列
bool CSVProcessor::removeColumn(int columnIndex) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return false;
    }
    
    // 删除表头
    m_header.erase(m_header.begin() + columnIndex);
    
    // 删除数据
    for (auto& row : m_data) {
        if (static_cast<size_t>(columnIndex) < row.size()) {
            row.erase(row.begin() + columnIndex);
        }
    }
    
    invalidateColumnIndexCache();
    return true;
}

bool CSVProcessor::removeColumn(const std::string& columnName) {
    int idx = getColumnIndex(columnName);
    if (idx >= 0) {
        return removeColumn(idx);
    }
    return false;
}

// 数据转换 - 重命名列
bool CSVProcessor::renameColumn(int columnIndex, const std::string& newName) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return false;
    }
    
    m_header[columnIndex] = newName;
    invalidateColumnIndexCache();
    return true;
}

bool CSVProcessor::renameColumn(const std::string& oldName, const std::string& newName) {
    int idx = getColumnIndex(oldName);
    if (idx >= 0) {
        return renameColumn(idx, newName);
    }
    return false;
}

// 数据转换 - 字符串替换
void CSVProcessor::replaceString(const std::string& search, const std::string& replace) {
    for (auto& row : m_data) {
        for (auto& field : row) {
            size_t pos = 0;
            while ((pos = field.find(search, pos)) != std::string::npos) {
                field.replace(pos, search.length(), replace);
                pos += replace.length();
            }
        }
    }
}

void CSVProcessor::replaceStringInColumn(int columnIndex, const std::string& search, const std::string& replace) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return;
    }
    
    for (auto& row : m_data) {
        if (static_cast<size_t>(columnIndex) < row.size()) {
            size_t pos = 0;
            while ((pos = row[columnIndex].find(search, pos)) != std::string::npos) {
                row[columnIndex].replace(pos, search.length(), replace);
                pos += replace.length();
            }
        }
    }
}

void CSVProcessor::replaceStringInColumn(const std::string& columnName, const std::string& search, const std::string& replace) {
    int idx = getColumnIndex(columnName);
    if (idx >= 0) {
        replaceStringInColumn(idx, search, replace);
    }
}

// 排序
int CSVProcessor::compareValues(const std::string& a, const std::string& b) const {
    // 尝试数值比较
    try {
        size_t posA, posB;
        double numA = std::stod(a, &posA);
        double numB = std::stod(b, &posB);
        
        if (posA == a.size() && posB == b.size()) {
            if (numA < numB) return -1;
            if (numA > numB) return 1;
            return 0;
        }
    } catch (...) {
        // 不是数值，继续字符串比较
    }
    
    // 字符串比较
    return a.compare(b);
}

void CSVProcessor::sortByColumn(int columnIndex, bool ascending) {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return;
    }
    
    std::sort(m_data.begin(), m_data.end(), 
        [this, columnIndex, ascending](const std::vector<std::string>& a, const std::vector<std::string>& b) {
            std::string valA = (static_cast<size_t>(columnIndex) < a.size()) ? a[columnIndex] : "";
            std::string valB = (static_cast<size_t>(columnIndex) < b.size()) ? b[columnIndex] : "";
            
            int cmp = compareValues(valA, valB);
            if (ascending) {
                return cmp < 0;
            } else {
                return cmp > 0;
            }
        }
    );
}

void CSVProcessor::sortByColumn(const std::string& columnName, bool ascending) {
    int idx = getColumnIndex(columnName);
    if (idx >= 0) {
        sortByColumn(idx, ascending);
    }
}

// 辅助方法
int CSVProcessor::getColumnIndex(const std::string& columnName) const {
    auto it = m_columnIndexCache.find(columnName);
    if (it != m_columnIndexCache.end()) {
        return it->second;
    }
    
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (m_header[i] == columnName) {
            m_columnIndexCache[columnName] = static_cast<int>(i);
            return static_cast<int>(i);
        }
    }
    
    m_columnIndexCache[columnName] = -1;
    return -1;
}

std::string CSVProcessor::getColumnName(int columnIndex) const {
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return "";
    }
    return m_header[columnIndex];
}

size_t CSVProcessor::getRowCount() const {
    return m_data.size();
}

size_t CSVProcessor::getColumnCount() const {
    return m_header.size();
}

void CSVProcessor::printInfo() const {
    std::cout << "CSV Information:" << std::endl;
    std::cout << "  Rows: " << m_data.size() << std::endl;
    std::cout << "  Columns: " << m_header.size() << std::endl;
    std::cout << "  Headers: ";
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << m_header[i];
    }
    std::cout << std::endl;
}

// 设置验证规则
void CSVProcessor::setRequiredColumns(const std::vector<std::string>& columns) {
    m_requiredColumns = columns;
}

void CSVProcessor::setColumnTypes(const std::map<std::string, std::string>& types) {
    m_columnTypes = types;
}

void CSVProcessor::clearValidationRules() {
    m_requiredColumns.clear();
    m_columnTypes.clear();
}

namespace {

enum TokenType {
    TOKEN_OPERATOR,
    TOKEN_COLUMN,
    TOKEN_NUMBER,
    TOKEN_INVALID
};

struct ExpressionToken {
    TokenType type;
    std::string value;
    double numValue;
    char op;
    
    ExpressionToken() : type(TOKEN_INVALID), numValue(0.0), op(0) {}
};

int getOperatorPrecedence(char op) {
    if (op == '*' || op == '/') {
        return 2;
    } else if (op == '+' || op == '-') {
        return 1;
    }
    return 0;
}

double applyOperator(double left, char op, double right) {
    switch (op) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/':
            if (right == 0.0) {
                throw std::runtime_error("Division by zero");
            }
            return left / right;
        default:
            throw std::runtime_error("Invalid operator");
    }
}

double getTokenValue(const ExpressionToken& tok, 
                     const std::vector<std::string>& row,
                     const std::vector<std::string>& header) {
    if (tok.type == TOKEN_NUMBER) {
        return tok.numValue;
    } else if (tok.type == TOKEN_COLUMN) {
        int colIdx = -1;
        for (size_t i = 0; i < header.size(); ++i) {
            if (header[i] == tok.value) {
                colIdx = static_cast<int>(i);
                break;
            }
        }
        
        if (colIdx < 0) {
            throw std::runtime_error("Column not found: " + tok.value);
        }
        
        if (static_cast<size_t>(colIdx) >= row.size()) {
            throw std::runtime_error("Row missing value for column: " + tok.value);
        }
        
        const std::string& cellValue = row[colIdx];
        if (cellValue.empty()) {
            throw std::runtime_error("Empty value in column: " + tok.value);
        }
        
        try {
            size_t pos;
            double value = std::stod(cellValue, &pos);
            if (pos != cellValue.size()) {
                throw std::runtime_error("Invalid numeric value: " + cellValue);
            }
            return value;
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Invalid numeric value: " + cellValue);
        }
    }
    return 0.0;
}

std::vector<ExpressionToken> tokenizeExpression(const std::string& expr) {
    std::vector<ExpressionToken> tokens;
    std::string current;
    bool inQuotes = false;
    
    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];
        
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            if (!current.empty()) {
                ExpressionToken tok;
                try {
                    size_t pos;
                    double num = std::stod(current, &pos);
                    if (pos == current.size()) {
                        tok.type = TOKEN_NUMBER;
                        tok.numValue = num;
                    } else {
                        tok.type = TOKEN_COLUMN;
                        tok.value = current;
                    }
                } catch (...) {
                    tok.type = TOKEN_COLUMN;
                    tok.value = current;
                }
                tokens.push_back(tok);
                current.clear();
            }
            
            ExpressionToken opTok;
            opTok.type = TOKEN_OPERATOR;
            opTok.op = c;
            tokens.push_back(opTok);
        }
        else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        ExpressionToken tok;
        try {
            size_t pos;
            double num = std::stod(current, &pos);
            if (pos == current.size()) {
                tok.type = TOKEN_NUMBER;
                tok.numValue = num;
            } else {
                tok.type = TOKEN_COLUMN;
                tok.value = current;
            }
        } catch (...) {
            tok.type = TOKEN_COLUMN;
            tok.value = current;
        }
        tokens.push_back(tok);
    }
    
    return tokens;
}

double evaluateExpression(const std::vector<ExpressionToken>& tokens, 
                          const std::vector<std::string>& row,
                          const std::vector<std::string>& header) {
    if (tokens.empty()) {
        return 0.0;
    }
    
    if (tokens.size() == 1) {
        return getTokenValue(tokens[0], row, header);
    }
    
    std::vector<double> values;
    std::vector<char> operators;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const ExpressionToken& tok = tokens[i];
        
        if (tok.type == TOKEN_OPERATOR) {
            operators.push_back(tok.op);
        } else {
            values.push_back(getTokenValue(tok, row, header));
        }
    }
    
    if (values.empty()) {
        return 0.0;
    }
    
    for (size_t i = 0; i < operators.size(); ) {
        if (operators[i] == '*' || operators[i] == '/') {
            double result = applyOperator(values[i], operators[i], values[i + 1]);
            values[i] = result;
            values.erase(values.begin() + i + 1);
            operators.erase(operators.begin() + i);
        } else {
            ++i;
        }
    }
    
    double result = values[0];
    for (size_t i = 0; i < operators.size(); ++i) {
        result = applyOperator(result, operators[i], values[i + 1]);
    }
    
    return result;
}

} // 匿名命名空间

// 计算列功能
bool CSVProcessor::addCalculatedColumn(const std::string& newColumnName, const std::string& expression) {
    return addCalculatedColumn(static_cast<int>(m_header.size()), newColumnName, expression);
}

bool CSVProcessor::addCalculatedColumn(int position, const std::string& newColumnName, const std::string& expression) {
    if (position < 0 || static_cast<size_t>(position) > m_header.size()) {
        return false;
    }
    
    // 先解析表达式，验证语法
    std::vector<ExpressionToken> tokens = tokenizeExpression(expression);
    if (tokens.empty()) {
        std::cerr << "Error: Empty or invalid expression" << std::endl;
        return false;
    }
    
    // 验证表达式中的列名是否存在
    for (const auto& tok : tokens) {
        if (tok.type == TOKEN_COLUMN) {
            bool found = false;
            for (const auto& h : m_header) {
                if (h == tok.value) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "Error: Column not found in expression: " << tok.value << std::endl;
                return false;
            }
        }
    }
    
    // 计算每一行的值
    std::vector<std::string> newColumn;
    bool hasError = false;
    
    for (size_t rowIdx = 0; rowIdx < m_data.size(); ++rowIdx) {
        const auto& row = m_data[rowIdx];
        try {
            double result = evaluateExpression(tokens, row, m_header);
            std::ostringstream ss;
            ss << result;
            newColumn.push_back(ss.str());
        } catch (const std::exception& e) {
            std::cerr << "Error calculating row " << (rowIdx + 1) << ": " << e.what() << std::endl;
            newColumn.push_back("");
            hasError = true;
        }
    }
    
    // 插入新列
    m_header.insert(m_header.begin() + position, newColumnName);
    
    for (size_t i = 0; i < m_data.size(); ++i) {
        if (static_cast<size_t>(position) <= m_data[i].size()) {
            m_data[i].insert(m_data[i].begin() + position, newColumn[i]);
        } else {
            while (m_data[i].size() < static_cast<size_t>(position)) {
                m_data[i].push_back("");
            }
            m_data[i].push_back(newColumn[i]);
        }
    }
    
    if (hasError) {
        std::cerr << "Warning: Some rows had calculation errors" << std::endl;
    }
    
    return true;
}

// 统计功能
ColumnStatistics CSVProcessor::calculateStatistics(const std::string& columnName) const {
    int idx = getColumnIndex(columnName);
    if (idx < 0) {
        ColumnStatistics stats;
        stats.columnName = columnName;
        return stats;
    }
    return calculateStatistics(idx);
}

ColumnStatistics CSVProcessor::calculateStatistics(int columnIndex) const {
    ColumnStatistics stats;
    
    if (columnIndex < 0 || static_cast<size_t>(columnIndex) >= m_header.size()) {
        return stats;
    }
    
    stats.columnName = m_header[columnIndex];
    stats.totalRows = m_data.size();
    
    std::vector<double> numericValues;
    std::unordered_map<std::string, size_t> valueCounts;
    size_t nonEmptyCount = 0;
    size_t missingCount = 0;
    
    size_t textMinLength = SIZE_MAX;
    size_t textMaxLength = 0;
    size_t textTotalLength = 0;
    size_t textCount = 0;
    
    double minVal = 0.0;
    double maxVal = 0.0;
    double sumVal = 0.0;
    size_t validCount = 0;
    bool firstValue = true;
    
    // Welford 在线算法变量
    double mean = 0.0;
    double M2 = 0.0;  // 平方差和
    
    for (const auto& row : m_data) {
        const std::string& value = 
            (static_cast<size_t>(columnIndex) < row.size()) ? row[columnIndex] : "";
        
        if (value.empty() || isMissingValue(value)) {
            missingCount++;
            continue;
        }
        
        nonEmptyCount++;
        valueCounts[value]++;
        
        textCount++;
        size_t len = value.length();
        textTotalLength += len;
        if (len < textMinLength) textMinLength = len;
        if (len > textMaxLength) textMaxLength = len;
        
        try {
            size_t pos;
            double num = std::stod(value, &pos);
            if (pos != value.size()) {
                continue;
            }
            
            if (firstValue) {
                minVal = num;
                maxVal = num;
                firstValue = false;
            } else {
                if (num < minVal) minVal = num;
                if (num > maxVal) maxVal = num;
            }
            
            sumVal += num;
            validCount++;
            numericValues.push_back(num);
            
            // Welford 在线算法更新
            double delta = num - mean;
            mean += delta / static_cast<double>(validCount);
            double delta2 = num - mean;
            M2 += delta * delta2;
        } catch (...) {
            continue;
        }
    }
    
    stats.nonEmptyCount = nonEmptyCount;
    stats.uniqueCount = valueCounts.size();
    stats.missingCount = missingCount;
    stats.validNumericRows = validCount;
    stats.hasValidData = (validCount > 0);
    
    if (stats.hasValidData) {
        stats.minValue = minVal;
        stats.maxValue = maxVal;
        stats.sum = sumVal;
        stats.average = sumVal / static_cast<double>(validCount);
        
        // 使用 nth_element 计算中位数（比 sort 更快，O(n) vs O(n log n)）
        if (validCount % 2 == 1) {
            // 奇数个元素：取中间位置
            std::nth_element(numericValues.begin(), 
                            numericValues.begin() + validCount / 2, 
                            numericValues.end());
            stats.median = numericValues[validCount / 2];
        } else {
            // 偶数个元素：需要找到第 n/2-1 和 n/2 个元素
            // 先找第 n/2 个元素
            size_t mid = validCount / 2;
            std::nth_element(numericValues.begin(), 
                            numericValues.begin() + mid, 
                            numericValues.end());
            double upperMid = numericValues[mid];
            
            // 再找第 n/2-1 个元素（在左半部分找最大值）
            std::nth_element(numericValues.begin(), 
                            numericValues.begin() + mid - 1, 
                            numericValues.begin() + mid);
            double lowerMid = numericValues[mid - 1];
            
            stats.median = (lowerMid + upperMid) / 2.0;
        }
        
        // 使用 Welford 算法计算方差和标准差
        if (validCount > 1) {
            stats.variance = M2 / static_cast<double>(validCount);  // 总体方差
            stats.standardDeviation = std::sqrt(stats.variance);
        } else {
            stats.variance = 0.0;
            stats.standardDeviation = 0.0;
        }
    }
    
    stats.hasTextData = (textCount > 0);
    if (stats.hasTextData) {
        stats.minLength = textMinLength;
        stats.maxLength = textMaxLength;
        stats.avgLength = static_cast<double>(textTotalLength) / static_cast<double>(textCount);
        
        size_t maxCount = 0;
        std::string modeValue = "";
        for (const auto& pair : valueCounts) {
            if (pair.second > maxCount) {
                maxCount = pair.second;
                modeValue = pair.first;
            }
        }
        stats.mode = modeValue;
        stats.modeCount = maxCount;
    }
    
    return stats;
}

std::map<std::string, ColumnStatistics> CSVProcessor::calculateAllStatistics() const {
    std::map<std::string, ColumnStatistics> result;
    
    for (const auto& colName : m_header) {
        result[colName] = calculateStatistics(colName);
    }
    
    return result;
}

void CSVProcessor::printStatistics(const std::string& columnName) const {
    ColumnStatistics stats = calculateStatistics(columnName);
    
    std::cout << "Statistics for column: " << stats.columnName << std::endl;
    std::cout << "  Total rows: " << stats.totalRows << std::endl;
    std::cout << "  Non-empty rows: " << stats.nonEmptyCount << std::endl;
    std::cout << "  Missing rows: " << stats.missingCount << std::endl;
    std::cout << "  Unique values: " << stats.uniqueCount << std::endl;
    
    if (stats.hasValidData) {
        std::cout << "\n  Numeric Statistics:" << std::endl;
        std::cout << "    Valid numeric rows: " << stats.validNumericRows << std::endl;
        std::cout << "    Minimum: " << stats.minValue << std::endl;
        std::cout << "    Maximum: " << stats.maxValue << std::endl;
        std::cout << "    Sum: " << stats.sum << std::endl;
        std::cout << "    Average (Mean): " << stats.average << std::endl;
        std::cout << "    Median: " << stats.median << std::endl;
        std::cout << "    Variance: " << stats.variance << std::endl;
        std::cout << "    Standard Deviation: " << stats.standardDeviation << std::endl;
    }
    
    if (stats.hasTextData) {
        std::cout << "\n  Text Statistics:" << std::endl;
        std::cout << "    Mode: '" << stats.mode << "' (appeared " << stats.modeCount << " times)" << std::endl;
        std::cout << "    Min length: " << stats.minLength << std::endl;
        std::cout << "    Max length: " << stats.maxLength << std::endl;
        std::cout << "    Average length: " << stats.avgLength << std::endl;
    }
    
    if (!stats.hasValidData && !stats.hasTextData) {
        std::cout << "  No valid data found" << std::endl;
    }
}

void CSVProcessor::printAllStatistics() const {
    for (const auto& colName : m_header) {
        printStatistics(colName);
        std::cout << std::endl;
    }
}

namespace {

std::string toLower(const std::string& s) {
    std::string result;
    for (char c : s) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

bool compareNumericValues(const std::string& a, const std::string& b, FilterOperator op) {
    try {
        double numA = std::stod(a);
        double numB = std::stod(b);
        
        switch (op) {
            case FilterOperator::EQUALS:
                return numA == numB;
            case FilterOperator::NOT_EQUALS:
                return numA != numB;
            case FilterOperator::GREATER_THAN:
                return numA > numB;
            case FilterOperator::LESS_THAN:
                return numA < numB;
            case FilterOperator::GREATER_OR_EQUAL:
                return numA >= numB;
            case FilterOperator::LESS_OR_EQUAL:
                return numA <= numB;
            default:
                return false;
        }
    } catch (...) {
        return false;
    }
}

bool matchCondition(const std::string& cellValue, FilterOperator op, 
                    const std::string& filterValue, bool caseSensitive,
                    const std::regex* precompiledRegex = nullptr) {
    std::string a = caseSensitive ? cellValue : toLower(cellValue);
    std::string b = caseSensitive ? filterValue : toLower(filterValue);
    
    switch (op) {
        case FilterOperator::EQUALS:
            return a == b;
        case FilterOperator::NOT_EQUALS:
            return a != b;
        case FilterOperator::CONTAINS:
            return a.find(b) != std::string::npos;
        case FilterOperator::NOT_CONTAINS:
            return a.find(b) == std::string::npos;
        case FilterOperator::STARTS_WITH:
            return a.size() >= b.size() && a.substr(0, b.size()) == b;
        case FilterOperator::ENDS_WITH:
            return a.size() >= b.size() && a.substr(a.size() - b.size()) == b;
        case FilterOperator::GREATER_THAN:
        case FilterOperator::LESS_THAN:
        case FilterOperator::GREATER_OR_EQUAL:
        case FilterOperator::LESS_OR_EQUAL:
            return compareNumericValues(a, b, op);
        case FilterOperator::REGEX_MATCH:
            if (precompiledRegex) {
                return std::regex_match(cellValue, *precompiledRegex);
            }
            try {
                std::regex::flag_type flags = caseSensitive ? 
                    std::regex::ECMAScript : std::regex::icase;
                std::regex re(filterValue, flags);
                return std::regex_match(cellValue, re);
            } catch (...) {
                return false;
            }
        default:
            return false;
    }
}

struct RegexCache {
    std::unique_ptr<std::regex> re;
    bool valid;
    
    RegexCache() : re(nullptr), valid(false) {}
    
    void compile(const std::string& pattern, bool caseSensitive) {
        try {
            std::regex::flag_type flags = caseSensitive ? 
                std::regex::ECMAScript : std::regex::icase;
            re = std::make_unique<std::regex>(pattern, flags);
            valid = true;
        } catch (...) {
            valid = false;
        }
    }
};

std::string escapeJSON(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << c; break;
        }
    }
    return ss.str();
}

std::string escapeXML(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        switch (c) {
            case '&': ss << "&amp;"; break;
            case '<': ss << "&lt;"; break;
            case '>': ss << "&gt;"; break;
            case '"': ss << "&quot;"; break;
            case '\'': ss << "&apos;"; break;
            default: ss << c; break;
        }
    }
    return ss.str();
}

std::string escapeSQL(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        if (c == '\'') {
            ss << "''";
        } else {
            ss << c;
        }
    }
    return ss.str();
}

std::string escapeSQLIdentifier(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        if (c == '`') {
            ss << "``";
        } else {
            ss << c;
        }
    }
    return ss.str();
}

std::string escapeHTML(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        switch (c) {
            case '&': ss << "&amp;"; break;
            case '<': ss << "&lt;"; break;
            case '>': ss << "&gt;"; break;
            case '"': ss << "&quot;"; break;
            default: ss << c; break;
        }
    }
    return ss.str();
}

std::string formatValueForSQL(const std::string& value) {
    if (value.empty()) {
        return "NULL";
    }
    
    try {
        size_t pos;
        std::stod(value, &pos);
        if (pos == value.size()) {
            return value;
        }
    } catch (...) {
    }
    
    return "'" + escapeSQL(value) + "'";
}

} // 匿名命名空间

void CSVProcessor::filter(const FilterCondition& condition) {
    int colIdx = getColumnIndex(condition.columnName);
    if (colIdx < 0) {
        std::cerr << "Warning: Column not found: " << condition.columnName << std::endl;
        return;
    }
    
    std::vector<std::vector<std::string>> newData;
    
    RegexCache regexCache;
    if (condition.op == FilterOperator::REGEX_MATCH) {
        regexCache.compile(condition.value, condition.caseSensitive);
        if (!regexCache.valid) {
            std::cerr << "Warning: Invalid regex pattern: " << condition.value << std::endl;
            return;
        }
    }
    
    for (const auto& row : m_data) {
        if (static_cast<size_t>(colIdx) >= row.size()) {
            continue;
        }
        
        const std::string& cellValue = row[colIdx];
        
        if (matchCondition(cellValue, condition.op, condition.value, 
                          condition.caseSensitive, regexCache.re.get())) {
            newData.push_back(row);
        }
    }
    
    size_t removed = m_data.size() - newData.size();
    m_data = newData;
    
    std::cout << "  Filtered by " << condition.columnName 
              << ", removed " << removed << " rows" << std::endl;
}

void CSVProcessor::filter(const std::vector<FilterCondition>& conditions, bool matchAll) {
    std::vector<std::vector<std::string>> newData;
    
    std::vector<std::pair<int, RegexCache>> columnIndices;
    std::vector<RegexCache> regexCaches;
    
    for (const auto& cond : conditions) {
        int colIdx = getColumnIndex(cond.columnName);
        columnIndices.emplace_back(colIdx, RegexCache());
        
        if (cond.op == FilterOperator::REGEX_MATCH) {
            columnIndices.back().second.compile(cond.value, cond.caseSensitive);
            if (!columnIndices.back().second.valid) {
                std::cerr << "Warning: Invalid regex pattern: " << cond.value << std::endl;
            }
        }
    }
    
    for (const auto& row : m_data) {
        bool matches = matchAll;
        
        for (size_t i = 0; i < conditions.size(); ++i) {
            const auto& cond = conditions[i];
            int colIdx = columnIndices[i].first;
            
            if (colIdx < 0) {
                if (matchAll) {
                    matches = false;
                    break;
                }
                continue;
            }
            
            if (static_cast<size_t>(colIdx) >= row.size()) {
                if (matchAll) {
                    matches = false;
                    break;
                }
                continue;
            }
            
            const std::string& cellValue = row[colIdx];
            bool rowMatches = matchCondition(cellValue, cond.op, cond.value, 
                                            cond.caseSensitive, columnIndices[i].second.re.get());
            
            if (matchAll) {
                if (!rowMatches) {
                    matches = false;
                    break;
                }
            } else {
                if (rowMatches) {
                    matches = true;
                    break;
                }
            }
        }
        
        if (matches) {
            newData.push_back(row);
        }
    }
    
    size_t removed = m_data.size() - newData.size();
    m_data = newData;
    
    std::cout << "  Applied " << conditions.size() << " filter conditions" 
              << ", removed " << removed << " rows" << std::endl;
}

void CSVProcessor::filterByRegex(const std::string& columnName, const std::string& pattern, bool caseSensitive) {
    FilterCondition cond;
    cond.columnName = columnName;
    cond.op = FilterOperator::REGEX_MATCH;
    cond.value = pattern;
    cond.caseSensitive = caseSensitive;
    filter(cond);
}

void CSVProcessor::filterByRegex(const std::string& pattern, bool caseSensitive) {
    std::vector<std::vector<std::string>> newData;
    
    std::regex::flag_type flags = caseSensitive ? 
        std::regex::ECMAScript : std::regex::icase;
    
    try {
        std::regex re(pattern, flags);
        
        for (const auto& row : m_data) {
            bool matches = false;
            
            for (const auto& cellValue : row) {
                if (std::regex_match(cellValue, re)) {
                    matches = true;
                    break;
                }
            }
            
            if (matches) {
                newData.push_back(row);
            }
        }
    } catch (...) {
        std::cerr << "Warning: Invalid regex pattern: " << pattern << std::endl;
    }
    
    size_t removed = m_data.size() - newData.size();
    m_data = newData;
    
    std::cout << "  Filtered by regex (all columns)" 
              << ", removed " << removed << " rows" << std::endl;
}

std::string CSVProcessor::toCSV() const {
    std::ostringstream ss;
    
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (i > 0) ss << m_delimiter;
        ss << formatCSVField(m_header[i]);
    }
    ss << '\n';
    
    for (const auto& row : m_data) {
        for (size_t i = 0; i < m_header.size(); ++i) {
            if (i > 0) ss << m_delimiter;
            if (i < row.size()) {
                ss << formatCSVField(row[i]);
            }
        }
        ss << '\n';
    }
    
    return ss.str();
}

std::string CSVProcessor::toJSON() const {
    std::ostringstream ss;
    ss << "[\n";
    
    for (size_t rowIdx = 0; rowIdx < m_data.size(); ++rowIdx) {
        const auto& row = m_data[rowIdx];
        ss << "  {\n";
        
        for (size_t colIdx = 0; colIdx < m_header.size(); ++colIdx) {
            ss << "    \"" << escapeJSON(m_header[colIdx]) << "\": ";
            if (colIdx < row.size()) {
                ss << "\"" << escapeJSON(row[colIdx]) << "\"";
            } else {
                ss << "null";
            }
            if (colIdx < m_header.size() - 1) {
                ss << ",";
            }
            ss << "\n";
        }
        
        ss << "  }";
        if (rowIdx < m_data.size() - 1) {
            ss << ",";
        }
        ss << "\n";
    }
    
    ss << "]";
    return ss.str();
}

std::string CSVProcessor::toXML() const {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<data>\n";
    
    for (const auto& row : m_data) {
        ss << "  <row>\n";
        
        for (size_t colIdx = 0; colIdx < m_header.size(); ++colIdx) {
            ss << "    <" << escapeXML(m_header[colIdx]) << ">";
            if (colIdx < row.size()) {
                ss << escapeXML(row[colIdx]);
            }
            ss << "</" << escapeXML(m_header[colIdx]) << ">\n";
        }
        
        ss << "  </row>\n";
    }
    
    ss << "</data>";
    return ss.str();
}

std::string CSVProcessor::toMarkdown() const {
    std::ostringstream ss;
    
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (i > 0) ss << " | ";
        ss << m_header[i];
    }
    ss << "\n";
    
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (i > 0) ss << " | ";
        ss << "---";
    }
    ss << "\n";
    
    for (const auto& row : m_data) {
        for (size_t i = 0; i < m_header.size(); ++i) {
            if (i > 0) ss << " | ";
            if (i < row.size()) {
                ss << row[i];
            }
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string CSVProcessor::toHTML() const {
    std::ostringstream ss;
    ss << "<!DOCTYPE html>\n";
    ss << "<html>\n";
    ss << "<head>\n";
    ss << "  <meta charset=\"UTF-8\">\n";
    ss << "  <title>CSV Data</title>\n";
    ss << "  <style>\n";
    ss << "    table { border-collapse: collapse; width: 100%; }\n";
    ss << "    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    ss << "    th { background-color: #4CAF50; color: white; }\n";
    ss << "    tr:nth-child(even) { background-color: #f2f2f2; }\n";
    ss << "  </style>\n";
    ss << "</head>\n";
    ss << "<body>\n";
    ss << "  <table>\n";
    
    ss << "    <thead>\n";
    ss << "      <tr>\n";
    for (const auto& h : m_header) {
        ss << "        <th>" << escapeHTML(h) << "</th>\n";
    }
    ss << "      </tr>\n";
    ss << "    </thead>\n";
    
    ss << "    <tbody>\n";
    for (const auto& row : m_data) {
        ss << "      <tr>\n";
        for (size_t i = 0; i < m_header.size(); ++i) {
            ss << "        <td>";
            if (i < row.size()) {
                ss << escapeHTML(row[i]);
            }
            ss << "</td>\n";
        }
        ss << "      </tr>\n";
    }
    ss << "    </tbody>\n";
    
    ss << "  </table>\n";
    ss << "</body>\n";
    ss << "</html>";
    return ss.str();
}

std::string CSVProcessor::toSQL(const std::string& tableName) const {
    std::ostringstream ss;
    std::string escapedTableName = escapeSQLIdentifier(tableName);
    
    ss << "CREATE TABLE IF NOT EXISTS `" << escapedTableName << "` (\n";
    for (size_t i = 0; i < m_header.size(); ++i) {
        ss << "  `" << escapeSQLIdentifier(m_header[i]) << "` TEXT";
        if (i < m_header.size() - 1) {
            ss << ",";
        }
        ss << "\n";
    }
    ss << ");\n\n";
    
    
    for (const auto& row : m_data) {
        ss << "INSERT INTO `" << escapedTableName << "` (";
        for (size_t i = 0; i < m_header.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "`" << escapeSQLIdentifier(m_header[i]) << "`";
        }
        ss << ") VALUES (";
        
        for (size_t i = 0; i < m_header.size(); ++i) {
            if (i > 0) ss << ", ";
            if (i < row.size()) {
                ss << formatValueForSQL(row[i]);
            } else {
                ss << "NULL";
            }
        }
        ss << ");\n";
    }
    
    return ss.str();
}

bool CSVProcessor::exportToFile(const std::string& filename, ExportFormat format, const std::string& tableName) const {
    return exportToFile(filename, format, m_header, tableName);
}

bool CSVProcessor::exportToFile(const std::string& filename, ExportFormat format, 
                                 const std::vector<std::string>& columns, const std::string& tableName) const {
    CSVProcessor temp;
    temp.m_delimiter = m_delimiter;
    temp.m_header = columns;
    
    std::vector<int> colIndices;
    for (const auto& col : columns) {
        int idx = getColumnIndex(col);
        if (idx < 0) {
            std::cerr << "Warning: Column not found: " << col << std::endl;
            return false;
        }
        colIndices.push_back(idx);
    }
    
    for (const auto& row : m_data) {
        std::vector<std::string> newRow;
        for (int idx : colIndices) {
            if (static_cast<size_t>(idx) < row.size()) {
                newRow.push_back(row[idx]);
            } else {
                newRow.push_back("");
            }
        }
        temp.m_data.push_back(newRow);
    }
    
    std::string content;
    switch (format) {
        case ExportFormat::CSV:
            content = temp.toCSV();
            break;
        case ExportFormat::JSON:
            content = temp.toJSON();
            break;
        case ExportFormat::XML:
            content = temp.toXML();
            break;
        case ExportFormat::MARKDOWN:
            content = temp.toMarkdown();
            break;
        case ExportFormat::HTML:
            content = temp.toHTML();
            break;
        case ExportFormat::SQL:
            {
                std::string actualTableName = tableName;
                if (actualTableName.empty()) {
                    size_t lastDot = filename.find_last_of(".");
                    actualTableName = "data";
                    if (lastDot != std::string::npos && lastDot > 0) {
                        size_t lastSep = filename.find_last_of("/\\");
                        size_t start = (lastSep == std::string::npos) ? 0 : lastSep + 1;
                        actualTableName = filename.substr(start, lastDot - start);
                    }
                }
                content = temp.toSQL(actualTableName);
            }
            break;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.close();
    
    return true;
}

bool CSVProcessor::mergeFrom(const CSVProcessor& other, bool addSourceColumn, const std::string& sourceValue) {
    if (other.m_header.empty()) {
        return false;
    }
    
    if (m_header.empty()) {
        m_header = other.m_header;
        if (addSourceColumn) {
            m_header.push_back("_Source");
        }
    } else {
        std::map<std::string, int> headerMap;
        for (size_t i = 0; i < m_header.size(); ++i) {
            headerMap[m_header[i]] = static_cast<int>(i);
        }
        
        size_t originalSize = m_header.size();
        for (const auto& col : other.m_header) {
            if (headerMap.find(col) == headerMap.end()) {
                headerMap[col] = static_cast<int>(m_header.size());
                m_header.push_back(col);
            }
        }
        
        if (addSourceColumn && headerMap.find("_Source") == headerMap.end()) {
            m_header.push_back("_Source");
        }
        
        for (auto& row : m_data) {
            while (row.size() < m_header.size()) {
                row.push_back("");
            }
        }
    }
    
    for (const auto& otherRow : other.m_data) {
        std::vector<std::string> newRow(m_header.size(), "");
        
        for (size_t i = 0; i < other.m_header.size(); ++i) {
            for (size_t j = 0; j < m_header.size(); ++j) {
                if (m_header[j] == other.m_header[i]) {
                    if (i < otherRow.size()) {
                        newRow[j] = otherRow[i];
                    }
                    break;
                }
            }
        }
        
        if (addSourceColumn) {
            for (size_t j = 0; j < m_header.size(); ++j) {
                if (m_header[j] == "_Source") {
                    newRow[j] = sourceValue;
                    break;
                }
            }
        }
        
        m_data.push_back(newRow);
    }
    
    invalidateColumnIndexCache();
    return true;
}

bool CSVProcessor::mergeColumnsFrom(const CSVProcessor& other) {
    if (other.m_header.empty() || other.m_data.empty()) {
        return false;
    }
    
    size_t minRows = std::min(m_data.size(), other.m_data.size());
    
    for (const auto& col : other.m_header) {
        bool exists = false;
        for (const auto& existingCol : m_header) {
            if (existingCol == col) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_header.push_back(col);
        }
    }
    
    for (size_t i = 0; i < minRows; ++i) {
        for (const auto& cell : other.m_data[i]) {
            m_data[i].push_back(cell);
        }
        while (m_data[i].size() < m_header.size()) {
            m_data[i].push_back("");
        }
    }
    
    for (size_t i = minRows; i < m_data.size(); ++i) {
        while (m_data[i].size() < m_header.size()) {
            m_data[i].push_back("");
        }
    }
    
    for (size_t i = minRows; i < other.m_data.size(); ++i) {
        std::vector<std::string> newRow;
        while (newRow.size() < m_header.size() - other.m_header.size()) {
            newRow.push_back("");
        }
        for (const auto& cell : other.m_data[i]) {
            newRow.push_back(cell);
        }
        while (newRow.size() < m_header.size()) {
            newRow.push_back("");
        }
        m_data.push_back(newRow);
    }
    
    invalidateColumnIndexCache();
    return true;
}

std::vector<std::string> CSVProcessor::findCSVFiles(const std::string& directory, bool recursive) {
    std::vector<std::string> result;
    
    try {
        namespace fs = std::filesystem;
        fs::path dirPath(directory);
        
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            return result;
        }
        
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".csv") {
                        result.push_back(entry.path().string());
                    }
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".csv") {
                        result.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace csvtool
