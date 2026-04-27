#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace csvtool {

struct Command {
    std::string name;
    std::vector<std::string> args;
    std::map<std::string, std::string> options;
};

class CommandParser {
public:
    CommandParser();
    ~CommandParser() = default;
    
    bool parse(int argc, char* argv[]);
    void printHelp() const;
    
    const std::vector<Command>& getCommands() const { return m_commands; }
    const std::string& getInputFile() const { return m_inputFile; }
    const std::string& getOutputFile() const { return m_outputFile; }
    char getDelimiter() const { return m_delimiter; }
    bool hasHelpFlag() const { return m_helpFlag; }
    bool hasVersionFlag() const { return m_versionFlag; }
    
private:
    std::vector<Command> m_commands;
    std::string m_inputFile;
    std::string m_outputFile;
    char m_delimiter;
    bool m_helpFlag;
    bool m_versionFlag;
    
    bool parseOption(const std::string& arg, const std::string& nextArg, bool& hasNextArg);
    Command parseCommand(const std::vector<std::string>& tokens);
    bool isCommand(const std::string& token) const;
    
    // 命令列表
    static const std::vector<std::string> s_commands;
};

} // namespace csvtool

#endif // COMMAND_PARSER_H
