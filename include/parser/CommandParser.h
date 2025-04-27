#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "core/Database.h"
#include "core/Table.h"

namespace soliddb {
namespace parser {

/**
 * Class to parse and execute SQL-like commands
 */
class CommandParser {
public:
    /**
     * Initialize a CommandParser
     */
    CommandParser();

    /**
     * Parse and execute a command string
     * @return true if the command was executed successfully
     */
    bool executeCommand(const std::string& command, std::shared_ptr<core::Database>& currentDatabase);

    /**
     * Print help information
     */
    void printHelp() const;

private:
    int operationCount;  // Counter for write operations since last checkpoint

    bool handleCreateDatabase(const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleCreateTable(const std::string& command, const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleUseDatabase(const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleInsert(const std::string& command, const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleSelect(const std::string& command, const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleListDatabases(const std::vector<std::string>& tokens);
    bool handleListTables(const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleSave(const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);
    bool handleRollback(const std::vector<std::string>& tokens, std::shared_ptr<core::Database>& currentDatabase);

    std::vector<std::string> tokenize(const std::string& input, char delimiter) const;
    std::vector<std::pair<std::string, std::string>> parseColumnDefinitions(const std::string& columnDefs) const;
    std::vector<core::ColumnDef> parseColumnDefsWithConstraints(const std::string& columnDefs) const;
    std::vector<std::string> parseValueList(const std::string& valueList) const;
};

} // namespace parser
} // namespace soliddb 