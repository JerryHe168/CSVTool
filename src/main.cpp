#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#include "csv_processor.h"
#include "command_parser.h"

using namespace csvtool;

const std::string VERSION = "1.0.0";

int main(int argc, char* argv[]) {
    try {
        // 设置控制台UTF-8编码（Windows专用）
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
        
        CommandParser parser;
        
        if (!parser.parse(argc, argv)) {
            std::cerr << "Error parsing command line arguments." << std::endl;
            return 1;
        }
        
        if (parser.hasHelpFlag()) {
            parser.printHelp();
            return 0;
        }
        
        if (parser.hasVersionFlag()) {
            std::cout << "CSV Tool version " << VERSION << std::endl;
            std::cout << "A cross-platform CSV file processing utility" << std::endl;
            return 0;
        }
        
        if (parser.getInputFile().empty()) {
            std::cerr << "Error: No input file specified." << std::endl;
            std::cerr << "Use -i or --input to specify an input CSV file." << std::endl;
            return 1;
        }
        
        // 创建CSV处理器并读取文件
        CSVProcessor processor;
        
        std::cout << "Reading CSV file: " << parser.getInputFile() << std::endl;
        if (!processor.read(parser.getInputFile(), parser.getDelimiter())) {
            std::cerr << "Error: Failed to read CSV file." << std::endl;
            return 1;
        }
        
        std::cout << "Successfully read " << processor.getRowCount() 
                  << " rows and " << processor.getColumnCount() << " columns." << std::endl;
        
        // 执行命令
        const std::vector<Command>& commands = parser.getCommands();
        bool hasExportCommand = false;
        
        for (const Command& cmd : commands) {
            std::cout << "Executing command: " << cmd.name << std::endl;
            
            if (cmd.name == "export") {
                hasExportCommand = true;
            }
            
            if (cmd.name == "info") {
                processor.printInfo();
            }
            else if (cmd.name == "validate") {
                std::vector<ValidationError> errors = processor.getValidationErrors();
                
                if (errors.empty()) {
                    std::cout << "Validation passed: No errors found." << std::endl;
                } else {
                    std::cout << "Validation found " << errors.size() << " error(s):" << std::endl;
                    for (const auto& err : errors) {
                        std::cout << "  Row " << err.row;
                        if (err.column >= 0) {
                            std::cout << ", Column " << err.column;
                        }
                        std::cout << ": " << err.message << std::endl;
                    }
                }
            }
            else if (cmd.name == "trim") {
                auto it = cmd.options.find("column");
                if (it != cmd.options.end()) {
                    processor.trimWhitespaceInColumn(it->second);
                    std::cout << "  Trimmed whitespace in column: " << it->second << std::endl;
                } else {
                    processor.trimWhitespace();
                    std::cout << "  Trimmed whitespace in all columns" << std::endl;
                }
            }
            else if (cmd.name == "remove-missing") {
                auto it = cmd.options.find("column");
                if (it != cmd.options.end()) {
                    size_t before = processor.getRowCount();
                    processor.removeRowsWithMissingValuesInColumn(it->second);
                    size_t after = processor.getRowCount();
                    std::cout << "  Removed " << (before - after) << " rows with missing values in column: " << it->second << std::endl;
                } else {
                    size_t before = processor.getRowCount();
                    processor.removeRowsWithMissingValues();
                    size_t after = processor.getRowCount();
                    std::cout << "  Removed " << (before - after) << " rows with missing values" << std::endl;
                }
            }
            else if (cmd.name == "remove-duplicates") {
                size_t before = processor.getRowCount();
                processor.removeDuplicateRows();
                size_t after = processor.getRowCount();
                std::cout << "  Removed " << (before - after) << " duplicate rows" << std::endl;
            }
            else if (cmd.name == "standardize-dates") {
                std::string format = "%Y-%m-%d";
                auto formatIt = cmd.options.find("format");
                if (formatIt != cmd.options.end()) {
                    format = formatIt->second;
                }
                
                auto colIt = cmd.options.find("column");
                if (colIt != cmd.options.end()) {
                    processor.standardizeDateFormatsInColumn(colIt->second, format);
                    std::cout << "  Standardized dates in column " << colIt->second << " to format: " << format << std::endl;
                } else {
                    processor.standardizeDateFormats(format);
                    std::cout << "  Standardized all dates to format: " << format << std::endl;
                }
            }
            else if (cmd.name == "add-column") {
                if (cmd.args.empty()) {
                    std::cerr << "Error: add-column requires a column name" << std::endl;
                    return 1;
                }
                
                std::string name = cmd.args[0];
                std::string defaultValue = "";
                int position = -1;
                
                auto defaultIt = cmd.options.find("default");
                if (defaultIt != cmd.options.end()) {
                    defaultValue = defaultIt->second;
                }
                
                auto posIt = cmd.options.find("position");
                if (posIt != cmd.options.end()) {
                    try {
                        position = std::stoi(posIt->second);
                    } catch (...) {
                        std::cerr << "Error: Invalid position value" << std::endl;
                        return 1;
                    }
                }
                
                bool success;
                if (position >= 0) {
                    success = processor.addColumn(position, name, defaultValue);
                } else {
                    success = processor.addColumn(name, defaultValue);
                }
                
                if (success) {
                    std::cout << "  Added column '" << name << "' with default value '" << defaultValue << "'" << std::endl;
                } else {
                    std::cerr << "Error: Failed to add column" << std::endl;
                    return 1;
                }
            }
            else if (cmd.name == "remove-column") {
                if (cmd.args.empty()) {
                    std::cerr << "Error: remove-column requires a column name" << std::endl;
                    return 1;
                }
                
                std::string name = cmd.args[0];
                if (processor.removeColumn(name)) {
                    std::cout << "  Removed column: " << name << std::endl;
                } else {
                    std::cerr << "Error: Column not found: " << name << std::endl;
                    return 1;
                }
            }
            else if (cmd.name == "rename-column") {
                if (cmd.args.size() < 2) {
                    std::cerr << "Error: rename-column requires old name and new name" << std::endl;
                    return 1;
                }
                
                std::string oldName = cmd.args[0];
                std::string newName = cmd.args[1];
                
                if (processor.renameColumn(oldName, newName)) {
                    std::cout << "  Renamed column '" << oldName << "' to '" << newName << "'" << std::endl;
                } else {
                    std::cerr << "Error: Column not found: " << oldName << std::endl;
                    return 1;
                }
            }
            else if (cmd.name == "replace") {
                if (cmd.args.size() < 2) {
                    std::cerr << "Error: replace requires search and replace strings" << std::endl;
                    return 1;
                }
                
                std::string search = cmd.args[0];
                std::string replace = cmd.args[1];
                
                auto colIt = cmd.options.find("column");
                if (colIt != cmd.options.end()) {
                    processor.replaceStringInColumn(colIt->second, search, replace);
                    std::cout << "  Replaced '" << search << "' with '" << replace << "' in column: " << colIt->second << std::endl;
                } else {
                    processor.replaceString(search, replace);
                    std::cout << "  Replaced '" << search << "' with '" << replace << "' in all columns" << std::endl;
                }
            }
            else if (cmd.name == "sort") {
                if (cmd.args.empty() && cmd.options.find("column") == cmd.options.end()) {
                    std::cerr << "Error: sort requires a column name" << std::endl;
                    return 1;
                }
                
                std::string column;
                if (!cmd.args.empty()) {
                    column = cmd.args[0];
                } else {
                    column = cmd.options.at("column");
                }
                
                bool ascending = true;
                auto descIt = cmd.options.find("descending");
                if (descIt != cmd.options.end()) {
                    ascending = false;
                }
                
                int colIdx = processor.getColumnIndex(column);
                if (colIdx < 0) {
                    std::cerr << "Error: Column not found: " << column << std::endl;
                    return 1;
                }
                
                processor.sortByColumn(colIdx, ascending);
                std::cout << "  Sorted by column '" << column << "' in " 
                          << (ascending ? "ascending" : "descending") << " order" << std::endl;
            }
            else if (cmd.name == "add-calc-column") {
                if (cmd.args.size() < 2) {
                    std::cerr << "Error: add-calc-column requires new column name and expression" << std::endl;
                    return 1;
                }
                
                std::string newName = cmd.args[0];
                std::string expression = cmd.args[1];
                int position = -1;
                
                auto posIt = cmd.options.find("position");
                if (posIt != cmd.options.end()) {
                    try {
                        position = std::stoi(posIt->second);
                    } catch (...) {
                        std::cerr << "Error: Invalid position value" << std::endl;
                        return 1;
                    }
                }
                
                bool success;
                if (position >= 0) {
                    success = processor.addCalculatedColumn(position, newName, expression);
                } else {
                    success = processor.addCalculatedColumn(newName, expression);
                }
                
                if (success) {
                    std::cout << "  Added calculated column '" << newName << "' with expression: " << expression << std::endl;
                } else {
                    std::cerr << "Error: Failed to add calculated column" << std::endl;
                    return 1;
                }
            }
            else if (cmd.name == "stats") {
                auto colIt = cmd.options.find("column");
                if (colIt != cmd.options.end()) {
                    processor.printStatistics(colIt->second);
                } else {
                    processor.printAllStatistics();
                }
            }
            else if (cmd.name == "filter") {
                if (cmd.args.size() < 3) {
                    std::cerr << "Error: filter requires COLUMN OP VALUE" << std::endl;
                    std::cerr << "       Operators: =, !=, >, <, >=, <=, contains, not-contains, starts-with, ends-with" << std::endl;
                    return 1;
                }
                
                std::string column = cmd.args[0];
                std::string opStr = cmd.args[1];
                std::string value = cmd.args[2];
                
                for (size_t i = 3; i < cmd.args.size(); ++i) {
                    value += " " + cmd.args[i];
                }
                
                FilterOperator op = FilterOperator::EQUALS;
                if (opStr == "=" || opStr == "==") {
                    op = FilterOperator::EQUALS;
                } else if (opStr == "!=" || opStr == "<>") {
                    op = FilterOperator::NOT_EQUALS;
                } else if (opStr == ">") {
                    op = FilterOperator::GREATER_THAN;
                } else if (opStr == "<") {
                    op = FilterOperator::LESS_THAN;
                } else if (opStr == ">=") {
                    op = FilterOperator::GREATER_OR_EQUAL;
                } else if (opStr == "<=") {
                    op = FilterOperator::LESS_OR_EQUAL;
                } else if (opStr == "contains") {
                    op = FilterOperator::CONTAINS;
                } else if (opStr == "not-contains") {
                    op = FilterOperator::NOT_CONTAINS;
                } else if (opStr == "starts-with") {
                    op = FilterOperator::STARTS_WITH;
                } else if (opStr == "ends-with") {
                    op = FilterOperator::ENDS_WITH;
                } else {
                    std::cerr << "Error: Unknown operator: " << opStr << std::endl;
                    return 1;
                }
                
                bool caseSensitive = true;
                auto ciIt = cmd.options.find("case-insensitive");
                if (ciIt != cmd.options.end()) {
                    caseSensitive = false;
                }
                
                FilterCondition condition;
                condition.columnName = column;
                condition.op = op;
                condition.value = value;
                condition.caseSensitive = caseSensitive;
                
                processor.filter(condition);
            }
            else if (cmd.name == "filter-regex") {
                if (cmd.args.size() < 2) {
                    std::cerr << "Error: filter-regex requires COLUMN PATTERN" << std::endl;
                    return 1;
                }
                
                std::string column = cmd.args[0];
                std::string pattern = cmd.args[1];
                for (size_t i = 2; i < cmd.args.size(); ++i) {
                    pattern += " " + cmd.args[i];
                }
                
                bool caseSensitive = true;
                auto ciIt = cmd.options.find("case-insensitive");
                if (ciIt != cmd.options.end()) {
                    caseSensitive = false;
                }
                
                processor.filterByRegex(column, pattern, caseSensitive);
            }
            else if (cmd.name == "export") {
                std::string outputFile = parser.getOutputFile();
                if (outputFile.empty()) {
                    std::cerr << "Error: export requires an output file (use -o or --output)" << std::endl;
                    return 1;
                }
                
                auto formatIt = cmd.options.find("format");
                if (formatIt == cmd.options.end()) {
                    std::cerr << "Error: export requires --format option" << std::endl;
                    std::cerr << "       Supported formats: csv, json, xml, md, markdown, html, sql" << std::endl;
                    return 1;
                }
                
                std::string formatStr = formatIt->second;
                ExportFormat format = ExportFormat::CSV;
                
                if (formatStr == "csv") {
                    format = ExportFormat::CSV;
                } else if (formatStr == "json") {
                    format = ExportFormat::JSON;
                } else if (formatStr == "xml") {
                    format = ExportFormat::XML;
                } else if (formatStr == "md" || formatStr == "markdown") {
                    format = ExportFormat::MARKDOWN;
                } else if (formatStr == "html") {
                    format = ExportFormat::HTML;
                } else if (formatStr == "sql") {
                    format = ExportFormat::SQL;
                } else {
                    std::cerr << "Error: Unknown export format: " << formatStr << std::endl;
                    return 1;
                }
                
                std::vector<std::string> columns;
                auto colsIt = cmd.options.find("columns");
                if (colsIt != cmd.options.end()) {
                    std::string colsStr = colsIt->second;
                    std::istringstream iss(colsStr);
                    std::string col;
                    while (std::getline(iss, col, ',')) {
                        if (!col.empty()) {
                            columns.push_back(col);
                        }
                    }
                }
                
                bool success;
                if (columns.empty()) {
                    success = processor.exportToFile(outputFile, format);
                } else {
                    success = processor.exportToFile(outputFile, format, columns);
                }
                
                if (success) {
                    std::cout << "  Exported to " << outputFile << " in " << formatStr << " format" << std::endl;
                } else {
                    std::cerr << "Error: Failed to export" << std::endl;
                    return 1;
                }
            }
            else {
                std::cerr << "Warning: Unknown command: " << cmd.name << std::endl;
            }
        }
        
        // 写入输出文件（如果指定且没有使用 export 命令）
        if (!parser.getOutputFile().empty() && !hasExportCommand) {
            std::cout << "Writing to output file: " << parser.getOutputFile() << std::endl;
            if (!processor.write(parser.getOutputFile(), parser.getDelimiter())) {
                std::cerr << "Error: Failed to write to output file." << std::endl;
                return 1;
            }
            std::cout << "Successfully wrote " << processor.getRowCount() 
                      << " rows to " << parser.getOutputFile() << std::endl;
        } else if (commands.empty()) {
            // 如果没有指定输出文件且没有命令，至少显示信息
            processor.printInfo();
        }
        
        std::cout << "Done." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: An unexpected error occurred." << std::endl;
        return 1;
    }
}
