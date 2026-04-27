#include "command_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace csvtool {

const std::vector<std::string> CommandParser::s_commands = {
    "trim",
    "remove-missing",
    "remove-duplicates",
    "standardize-dates",
    "add-column",
    "remove-column",
    "rename-column",
    "replace",
    "sort",
    "validate",
    "info"
};

CommandParser::CommandParser() 
    : m_delimiter(','), m_helpFlag(false), m_versionFlag(false) {}

bool CommandParser::parse(int argc, char* argv[]) {
    if (argc < 2) {
        m_helpFlag = true;
        return true;
    }
    
    std::vector<std::string> args(argv + 1, argv + argc);
    std::vector<std::string> commandTokens;
    bool inCommand = false;
    
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        bool hasNextArg = (i + 1 < args.size());
        
        if (arg == "--help" || arg == "-h") {
            m_helpFlag = true;
            return true;
        }
        else if (arg == "--version" || arg == "-v") {
            m_versionFlag = true;
            return true;
        }
        else if (arg == "-i" || arg == "--input") {
            if (!hasNextArg) {
                std::cerr << "Error: " << arg << " requires a file path" << std::endl;
                return false;
            }
            m_inputFile = args[++i];
        }
        else if (arg == "-o" || arg == "--output") {
            if (!hasNextArg) {
                std::cerr << "Error: " << arg << " requires a file path" << std::endl;
                return false;
            }
            m_outputFile = args[++i];
        }
        else if (arg == "-d" || arg == "--delimiter") {
            if (!hasNextArg) {
                std::cerr << "Error: " << arg << " requires a delimiter character" << std::endl;
                return false;
            }
            std::string delim = args[++i];
            if (delim == "\\t" || delim == "tab") {
                m_delimiter = '\t';
            } else if (delim == "comma") {
                m_delimiter = ',';
            } else if (delim == "semicolon") {
                m_delimiter = ';';
            } else if (delim.size() == 1) {
                m_delimiter = delim[0];
            } else {
                std::cerr << "Error: Invalid delimiter: " << delim << std::endl;
                return false;
            }
        }
        else if (isCommand(arg)) {
            // 完成之前的命令
            if (!commandTokens.empty()) {
                m_commands.push_back(parseCommand(commandTokens));
                commandTokens.clear();
            }
            commandTokens.push_back(arg);
            inCommand = true;
        }
        else if (inCommand) {
            commandTokens.push_back(arg);
        }
        else {
            // 可能是输入文件（当没有使用-i选项时）
            if (m_inputFile.empty()) {
                m_inputFile = arg;
            } else {
                std::cerr << "Error: Unknown argument: " << arg << std::endl;
                return false;
            }
        }
    }
    
    // 完成最后一个命令
    if (!commandTokens.empty()) {
        m_commands.push_back(parseCommand(commandTokens));
    }
    
    return true;
}

bool CommandParser::isCommand(const std::string& token) const {
    return std::find(s_commands.begin(), s_commands.end(), token) != s_commands.end();
}

Command CommandParser::parseCommand(const std::vector<std::string>& tokens) {
    Command cmd;
    if (tokens.empty()) return cmd;
    
    cmd.name = tokens[0];
    
    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        
        if (token.substr(0, 2) == "--") {
            // 选项
            size_t eqPos = token.find('=');
            std::string key, value;
            
            if (eqPos != std::string::npos) {
                key = token.substr(2, eqPos - 2);
                value = token.substr(eqPos + 1);
            } else {
                key = token.substr(2);
                if (i + 1 < tokens.size() && tokens[i + 1].substr(0, 2) != "--") {
                    value = tokens[++i];
                } else {
                    value = "true";
                }
            }
            
            cmd.options[key] = value;
        } else {
            // 参数
            cmd.args.push_back(token);
        }
    }
    
    return cmd;
}

void CommandParser::printHelp() const {
    std::cout << "CSV Tool - A cross-platform CSV file processing utility" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  csvtool [OPTIONS] [COMMANDS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help          Print this help message" << std::endl;
    std::cout << "  -v, --version       Print version information" << std::endl;
    std::cout << "  -i, --input FILE    Input CSV file (required)" << std::endl;
    std::cout << "  -o, --output FILE   Output CSV file (optional)" << std::endl;
    std::cout << "  -d, --delimiter C   Delimiter character (default: comma)" << std::endl;
    std::cout << "                      Use '\\t' or 'tab' for tab, 'semicolon' for semicolon" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  info                              Display CSV file information" << std::endl;
    std::cout << "  validate                          Validate CSV file structure and data" << std::endl;
    std::cout << "  trim [--column NAME]              Remove whitespace from all cells" << std::endl;
    std::cout << "  remove-missing [--column NAME]    Remove rows with missing values" << std::endl;
    std::cout << "  remove-duplicates                 Remove duplicate rows" << std::endl;
    std::cout << "  standardize-dates [--column NAME] [--format FMT]" << std::endl;
    std::cout << "                                    Standardize date formats (default: %Y-%m-%d)" << std::endl;
    std::cout << "  add-column NAME [--position N] [--default VALUE]" << std::endl;
    std::cout << "                                    Add a new column" << std::endl;
    std::cout << "  remove-column NAME                Remove a column by name" << std::endl;
    std::cout << "  rename-column OLD_NAME NEW_NAME   Rename a column" << std::endl;
    std::cout << "  replace SEARCH REPLACE [--column NAME]" << std::endl;
    std::cout << "                                    Replace string in cells" << std::endl;
    std::cout << "  sort COLUMN [--descending]        Sort by column (default: ascending)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  csvtool -i data.csv info" << std::endl;
    std::cout << "  csvtool -i data.csv -o cleaned.csv trim remove-missing sort --column=Name" << std::endl;
    std::cout << "  csvtool -i data.csv -o output.csv add-column Status --default=Active" << std::endl;
    std::cout << "  csvtool -i data.csv replace \"old\" \"new\" --column=Description" << std::endl;
    std::cout << std::endl;
}

} // namespace csvtool
