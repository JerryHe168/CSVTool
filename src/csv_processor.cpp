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
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) file << delimiter;
            if (i < m_header.size()) {
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

// 辅助函数：检查字符串是否只包含数字
bool isAllDigits(const std::string& s) {
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
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
    
    // 检查是否看起来像日期（包含数字和分隔符）
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
    
    // 尝试解析常见格式
    int year = 0, month = 0, day = 0;
    
    // 格式1: YYYY-MM-DD 或 YYYY/MM/DD 或 YYYY.MM.DD
    {
        std::regex pattern(R"(^(\d{4})[-/.](\d{1,2})[-/.](\d{1,2})$)");
        std::smatch match;
        if (std::regex_match(trimmed, match, pattern)) {
            year = std::stoi(match[1].str());
            month = std::stoi(match[2].str());
            day = std::stoi(match[3].str());
            
            if (isValidYear(year) && isValidMonth(month) && isValidDay(year, month, day)) {
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                return true;
            }
        }
    }
    
    // 格式2: DD-MM-YYYY 或 DD/MM/YYYY 或 DD.MM.YYYY
    {
        std::regex pattern(R"(^(\d{1,2})[-/.](\d{1,2})[-/.](\d{4})$)");
        std::smatch match;
        if (std::regex_match(trimmed, match, pattern)) {
            day = std::stoi(match[1].str());
            month = std::stoi(match[2].str());
            year = std::stoi(match[3].str());
            
            if (isValidYear(year) && isValidMonth(month) && isValidDay(year, month, day)) {
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                return true;
            }
        }
    }
    
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
    }
    
    // 格式4: "Jan 5, 2023" 或 "January 5, 2023"
    {
        std::regex pattern(R"(^([A-Za-z]+)\s+(\d{1,2}),\s+(\d{4})$)");
        std::smatch match;
        if (std::regex_match(trimmed, match, pattern)) {
            std::string monthName = match[1].str();
            month = monthNameToNumber(monthName);
            day = std::stoi(match[2].str());
            year = std::stoi(match[3].str());
            
            if (month > 0 && isValidYear(year) && isValidDay(year, month, day)) {
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                return true;
            }
        }
    }
    
    // 格式5: "5 Jan 2023" 或 "5 January 2023"
    {
        std::regex pattern(R"(^(\d{1,2})\s+([A-Za-z]+)\s+(\d{4})$)");
        std::smatch match;
        if (std::regex_match(trimmed, match, pattern)) {
            day = std::stoi(match[1].str());
            std::string monthName = match[2].str();
            month = monthNameToNumber(monthName);
            year = std::stoi(match[3].str());
            
            if (month > 0 && isValidYear(year) && isValidDay(year, month, day)) {
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                return true;
            }
        }
    }
    
    // 格式6: 带时间的日期 (只提取日期部分)
    {
        // YYYY-MM-DD HH:MM:SS
        std::regex pattern1(R"(^(\d{4})-(\d{1,2})-(\d{1,2})\s+\d{1,2}:\d{1,2}:\d{1,2}$)");
        std::smatch match;
        if (std::regex_match(trimmed, match, pattern1)) {
            year = std::stoi(match[1].str());
            month = std::stoi(match[2].str());
            day = std::stoi(match[3].str());
            
            if (isValidYear(year) && isValidMonth(month) && isValidDay(year, month, day)) {
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                return true;
            }
        }
        
        // YYYY-MM-DDTHH:MM:SS (ISO 8601)
        std::regex pattern2(R"(^(\d{4})-(\d{1,2})-(\d{1,2})T\d{1,2}:\d{1,2}:\d{1,2}$)");
        if (std::regex_match(trimmed, match, pattern2)) {
            year = std::stoi(match[1].str());
            month = std::stoi(match[2].str());
            day = std::stoi(match[3].str());
            
            if (isValidYear(year) && isValidMonth(month) && isValidDay(year, month, day)) {
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                return true;
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
    for (size_t i = 0; i < m_header.size(); ++i) {
        if (m_header[i] == columnName) {
            return static_cast<int>(i);
        }
    }
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

} // namespace csvtool
