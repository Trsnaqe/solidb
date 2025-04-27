#include "parser/CommandParser.h"
#include "util/StringUtils.h"
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
namespace soliddb {
namespace parser {

CommandParser::CommandParser() : operationCount(0) {
}

bool CommandParser::executeCommand(const std::string& command, std::shared_ptr<core::Database>& currentDatabase) {
    if (command.empty()) {
        return true;
    }
    
    std::vector<std::string> tokens = tokenize(command, ' ');
    if (tokens.empty()) {
        return true;
    }
    
    std::string cmd = util::StringUtils::toUpper(tokens[0]);
    bool result = true;
    bool isWriteOperation = false;
    
    if (cmd == "HELP") {
        printHelp();
    }
    else if (cmd == "EXIT") {
        if (currentDatabase) {
            std::cout << "Saving database before exit...\n";
            currentDatabase->checkpoint();
        }
        return false;
    }
    else if (cmd == "CREATE" && tokens.size() >= 3) {
        std::string type = util::StringUtils::toUpper(tokens[1]);
        
        if (type == "DATABASE") {
            result = handleCreateDatabase(tokens, currentDatabase);
            isWriteOperation = true;
        }
        else if (type == "TABLE") {
            result = handleCreateTable(command, tokens, currentDatabase);
            isWriteOperation = true;
        }
        else {
            std::cout << "Error: Invalid CREATE command. Use CREATE DATABASE or CREATE TABLE.\n";
        }
    }
    else if (cmd == "USE" && tokens.size() >= 2) {
        if (currentDatabase) {
            currentDatabase->saveToFile();
        }
        result = handleUseDatabase(tokens, currentDatabase);
    }
    else if (cmd == "INSERT" && tokens.size() >= 5) {
        result = handleInsert(command, tokens, currentDatabase);
        isWriteOperation = true;
    }
    else if (cmd == "SELECT") {
        result = handleSelect(command, tokens, currentDatabase);
    }
    else if (cmd == "LIST" && tokens.size() >= 2) {
        std::string type = util::StringUtils::toUpper(tokens[1]);
        
        if (type == "DATABASES") {
            result = handleListDatabases(tokens);
        }
        else if (type == "TABLES") {
            result = handleListTables(tokens, currentDatabase);
        }
        else {
            std::cout << "Error: Unknown LIST command. Use LIST DATABASES or LIST TABLES.\n";
        }
    }
    else if (cmd == "CHECKPOINT" || cmd == "SAVE" || cmd == "COMMIT") {
        result = handleSave(tokens, currentDatabase);
    }
    else if (cmd == "ROLLBACK") {
        result = handleRollback(tokens, currentDatabase);
    }
    else {
        std::cout << "Unknown or incomplete command. Type HELP for assistance.\n";
    }
    
    if (isWriteOperation && currentDatabase) {
        std::cout << "Operation logged to transaction log.\n";
        currentDatabase->logOperation(command);
        operationCount++;
        
        if (operationCount >= 5) {
            if (currentDatabase->checkpoint()) {
                std::cout << "Checkpoint: Database state persisted to disk.\n";
            } else {
                std::cout << "Warning: Checkpoint failed.\n";
            }
            operationCount = 0;
        }
    }
    
    return result;
}

void CommandParser::printHelp() const {
    std::cout << "SolidDB - Simple Relational Database\n";
    std::cout << "Available commands:\n";
    std::cout << "  CREATE DATABASE <name> - Create a new database\n";
    std::cout << "  USE <database> - Switch to the specified database\n";
    std::cout << "  CREATE TABLE <name> (<column1> <type1> [constraints], <column2> <type2> [constraints], ...) - Create a new table\n";
    std::cout << "      Column constraints: PRIMARY KEY, UNIQUE, NOT NULL\n";
    std::cout << "      Example: CREATE TABLE users (id INT PRIMARY KEY, name STRING NOT NULL, email STRING UNIQUE)\n";
    std::cout << "  INSERT INTO <table> VALUES (<value1>, <value2>, ...) - Insert a row into a table\n";
    std::cout << "  SELECT <column1>, <column2>, ... FROM <table> [WHERE <condition>] - Query data from a table\n";
    std::cout << "  LIST DATABASES - Show all available databases\n";
    std::cout << "  LIST TABLES - Show all tables in the current database\n";
    std::cout << "  COMMIT - Save all changes to disk (same as CHECKPOINT)\n";
    std::cout << "  ROLLBACK - Revert changes since last commit/checkpoint\n";
    std::cout << "  HELP - Show this help message\n";
    std::cout << "  EXIT - Exit the program\n";
    std::cout << "\nData Persistence:\n";
    std::cout << "  - Operations are logged immediately (Write-Ahead Logging)\n";
    std::cout << "  - Database state is checkpointed after every 5 write operations\n";
    std::cout << "  - Use COMMIT to save changes immediately\n";
    std::cout << "  - Use ROLLBACK to revert uncommitted changes\n";
    std::cout << "  - All changes are guaranteed to be saved when you exit\n";
}

bool CommandParser::handleCreateDatabase(const std::vector<std::string>& tokens, 
                                  std::shared_ptr<core::Database>& currentDatabase) {
    if (tokens.size() < 3) {
        std::cout << "Error: Missing database name.\n";
        return true;
    }
    
    std::string dbName = tokens[2];
    
    if (fs::exists(dbName) && fs::is_directory(dbName) && fs::exists(dbName + "/metadata.db")) {
        std::cout << "Database '" << dbName << "' already exists.\n";
    } else {
        currentDatabase = std::make_shared<core::Database>(dbName);
        std::cout << "Database '" << dbName << "' created successfully.\n";
    }
    
    return true;
}

bool CommandParser::handleCreateTable(const std::string& command, 
                               const std::vector<std::string>& tokens,
                               std::shared_ptr<core::Database>& currentDatabase) {
    if (!currentDatabase) {
        std::cout << "Error: No database selected. Use CREATE DATABASE or USE command first.\n";
        return true;
    }
    
    if (tokens.size() < 3) {
        std::cout << "Error: Invalid CREATE TABLE syntax.\n";
        return true;
    }
    
    std::string tableName = tokens[2];
    
    size_t openParenPos = command.find('(');
    size_t closeParenPos = command.find_last_of(')');
    
    if (openParenPos == std::string::npos || closeParenPos == std::string::npos || 
        openParenPos >= closeParenPos) {
        std::cout << "Error: Invalid table definition syntax.\n";
        return true;
    }
    
    std::string columnDefs = command.substr(openParenPos + 1, closeParenPos - openParenPos - 1);
    auto columns = parseColumnDefsWithConstraints(columnDefs);
    
    if (columns.empty()) {
        std::cout << "Error: No valid columns defined.\n";
        return true;
    }
    
    if (currentDatabase->createTable(tableName, columns)) {
        std::cout << "Table '" << tableName << "' created successfully.\n";
    } else {
        std::cout << "Error creating table '" << tableName << "'.\n";
    }
    
    return true;
}

bool CommandParser::handleUseDatabase(const std::vector<std::string>& tokens, 
                               std::shared_ptr<core::Database>& currentDatabase) {
    if (tokens.size() < 2) {
        std::cout << "Error: Missing database name.\n";
        return true;
    }
    
    std::string dbName = tokens[1];
    
    if (!fs::exists(dbName) || !fs::is_directory(dbName)) {
        std::cout << "Error: Database '" << dbName << "' does not exist.\n";
        handleListDatabases(tokens);
    } else {
        currentDatabase = std::shared_ptr<core::Database>(
            core::Database::loadFromFile(dbName).release());
        
        if (currentDatabase) {
            std::cout << "Using database '" << dbName << "'.\n";
        } else {
            std::cout << "Error: Could not load database '" << dbName << "'.\n";
        }
    }
    
    return true;
}

bool CommandParser::handleInsert(const std::string& command, 
                         const std::vector<std::string>& tokens,
                         std::shared_ptr<core::Database>& currentDatabase) {
    if (!currentDatabase) {
        std::cout << "Error: No database selected. Use CREATE DATABASE or USE command first.\n";
        return true;
    }
    
    if (tokens.size() < 5 || 
        util::StringUtils::toUpper(tokens[1]) != "INTO" || 
        util::StringUtils::toUpper(tokens[3]) != "VALUES") {
        std::cout << "Error: Invalid INSERT syntax.\n";
        return true;
    }
    
    std::string tableName = tokens[2];
    
    size_t openParenPos = command.find('(', command.find("VALUES"));
    size_t closeParenPos = command.find_last_of(')');
    
    if (openParenPos == std::string::npos || closeParenPos == std::string::npos || 
        openParenPos >= closeParenPos) {
        std::cout << "Error: Invalid INSERT syntax.\n";
        return true;
    }
    
    std::string valueStr = command.substr(openParenPos + 1, closeParenPos - openParenPos - 1);
    auto values = parseValueList(valueStr);
    
    if (currentDatabase->insert(tableName, values)) {
        std::cout << "Row inserted successfully.\n";
    } else {
        std::cout << "Error inserting row.\n";
    }
    
    return true;
}

bool CommandParser::handleSelect(const std::string& command, 
                         const std::vector<std::string>& tokens,
                         std::shared_ptr<core::Database>& currentDatabase) {
    if (!currentDatabase) {
        std::cout << "Error: No database selected. Use CREATE DATABASE or USE command first.\n";
        return true;
    }
    
    auto fromIt = std::find(tokens.begin(), tokens.end(), "FROM");
    if (fromIt == tokens.end() || std::distance(tokens.begin(), fromIt) <= 1) {
        std::cout << "Error: Invalid SELECT syntax. Missing FROM clause.\n";
        return true;
    }
    
    std::vector<std::string> columns;
    std::string columnsStr = command.substr(7, command.find(" FROM ") - 7);
    
    if (columnsStr == "*") {
    } else {
        columns = tokenize(columnsStr, ',');
    }
    
    std::string tableName = *(fromIt + 1);
    
    std::string whereCondition;
    auto whereIt = std::find(tokens.begin(), tokens.end(), "WHERE");
    if (whereIt != tokens.end() && std::distance(whereIt, tokens.end()) > 1) {
        whereCondition = command.substr(command.find("WHERE") + 6);
    }
    
    auto results = currentDatabase->select(tableName, columns, whereCondition);
    
    if (results.empty()) {
        std::cout << "No results found.\n";
    } else {
        for (const auto& row : results) {
            for (size_t i = 0; i < row.size(); i++) {
                std::cout << row[i];
                if (i < row.size() - 1) {
                    std::cout << " | ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << results.size() << " row(s) returned.\n";
    }
    
    return true;
}

bool CommandParser::handleListDatabases(const std::vector<std::string>& tokens) {
    std::cout << "Available databases:\n";
    
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_directory()) {
            std::string path = entry.path().filename().string();
            if (fs::exists(entry.path() / "metadata.db")) {
                std::cout << "  " << path << "\n";
            }
        }
    }
    
    return true;
}

bool CommandParser::handleListTables(const std::vector<std::string>& tokens, 
                             std::shared_ptr<core::Database>& currentDatabase) {
    if (!currentDatabase) {
        std::cout << "Error: No database selected. Use CREATE DATABASE or USE command first.\n";
        return true;
    }
    
    std::cout << "Tables in " << currentDatabase->getName() << ":\n";
    
    auto tableNames = currentDatabase->getTableNames();
    if (tableNames.empty()) {
        std::cout << "  No tables found.\n";
    } else {
        for (const auto& name : tableNames) {
            std::cout << "  " << name << "\n";
        }
    }
    
    return true;
}

bool CommandParser::handleSave(const std::vector<std::string>& tokens, 
                       std::shared_ptr<core::Database>& currentDatabase) {
    if (!currentDatabase) {
        std::cout << "Error: No database selected. Use CREATE DATABASE or USE command first.\n";
        return true;
    }
    
    if (currentDatabase->checkpoint()) {
        std::cout << "Changes committed to disk successfully.\n";
        operationCount = 0;
    } else {
        std::cout << "Error committing changes.\n";
    }
    
    return true;
}

bool CommandParser::handleRollback(const std::vector<std::string>& tokens,
                           std::shared_ptr<core::Database>& currentDatabase) {
    if (!currentDatabase) {
        std::cout << "Error: No database selected. Use CREATE DATABASE or USE command first.\n";
        return true;
    }
    
    std::string dbName = currentDatabase->getName();
    
    currentDatabase = std::shared_ptr<core::Database>(
        core::Database::loadFromFile(dbName).release());
    
    if (currentDatabase) {
        std::cout << "Changes rolled back successfully. Database restored to last committed state.\n";
        operationCount = 0;
    } else {
        std::cout << "Error rolling back changes. Could not reload database state.\n";
    }
    
    return true;
}

std::vector<std::string> CommandParser::tokenize(const std::string& input, char delimiter) const {
    return util::StringUtils::tokenize(input, delimiter);
}

std::vector<core::ColumnDef> CommandParser::parseColumnDefsWithConstraints(
        const std::string& columnDefs) const {
    std::vector<core::ColumnDef> columns;
    std::vector<std::string> columnParts = tokenize(columnDefs, ',');
    
    for (auto& part : columnParts) {
        part.erase(0, part.find_first_not_of(" \t"));
        part.erase(part.find_last_not_of(" \t") + 1);
        
        std::vector<std::string> colTokens = tokenize(part, ' ');
        if (colTokens.size() >= 2) {
            std::string colName = colTokens[0];
            std::string colType = colTokens[1];
            int constraints = 0;
            
            for (size_t i = 2; i < colTokens.size(); i++) {
                std::string constraint = util::StringUtils::toUpper(colTokens[i]);
                
                if (constraint == "PRIMARY" && i + 1 < colTokens.size() && 
                    util::StringUtils::toUpper(colTokens[i+1]) == "KEY") {
                    constraints |= static_cast<int>(core::ColumnConstraint::PRIMARY_KEY);
                    i++;
                }
                else if (constraint == "UNIQUE") {
                    constraints |= static_cast<int>(core::ColumnConstraint::UNIQUE);
                }
                else if (constraint == "NOT" && i + 1 < colTokens.size() && 
                         util::StringUtils::toUpper(colTokens[i+1]) == "NULL") {
                    constraints |= static_cast<int>(core::ColumnConstraint::NOT_NULL);
                    i++;
                }
            }
            
            columns.emplace_back(colName, colType, constraints);
        }
    }
    
    return columns;
}

std::vector<std::string> CommandParser::parseValueList(const std::string& valueList) const {
    return tokenize(valueList, ',');
}

} // namespace parser
} // namespace soliddb 