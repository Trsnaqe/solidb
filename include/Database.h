#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Table.h"

namespace soliddb {

/**
 * Main Database class that manages tables and provides query execution
 */
class Database {
public:
    /**
     * Create a new database with the given name
     */
    explicit Database(const std::string& name);

    /**
     * Create a new table in the database
     */
    bool createTable(const std::string& tableName, const std::vector<std::pair<std::string, std::string>>& columns);

    /**
     * Insert a row into a table
     */
    bool insert(const std::string& tableName, const std::vector<std::string>& values);

    /**
     * Select rows from a table with optional where condition
     * For simplicity, condition is a string like "column=value"
     */
    std::vector<std::vector<std::string>> select(
        const std::string& tableName, 
        const std::vector<std::string>& columns,
        const std::string& whereCondition = ""
    );

    /**
     * Get the database name
     */
    std::string getName() const;

    /**
     * Save database to disk
     */
    bool saveToFile() const;

    /**
     * Load database from disk
     */
    static std::unique_ptr<Database> loadFromFile(const std::string& name);

private:
    std::string name_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;
};

} // namespace soliddb 