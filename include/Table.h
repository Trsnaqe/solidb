#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace soliddb {

/**
 * Represents a table in the database
 */
class Table {
public:
    /**
     * Create a new table with the given name and column definitions
     */
    Table(const std::string& name, const std::vector<std::pair<std::string, std::string>>& columns);

    /**
     * Insert a row into the table
     */
    bool insertRow(const std::vector<std::string>& values);

    /**
     * Select rows from the table with optional where condition
     */
    std::vector<std::vector<std::string>> selectRows(
        const std::vector<std::string>& columns,
        const std::string& whereCondition = ""
    ) const;

    /**
     * Get the table name
     */
    std::string getName() const;

    /**
     * Get column definitions
     */
    const std::vector<std::pair<std::string, std::string>>& getColumns() const;

    /**
     * Serialize the table to a string for storage
     */
    std::string serialize() const;

    /**
     * Deserialize a table from string
     */
    static std::unique_ptr<Table> deserialize(const std::string& data);

private:
    std::string name_;
    std::vector<std::pair<std::string, std::string>> columns_; // column name, type
    std::vector<std::vector<std::string>> rows_;

    // Helper methods
    bool validateRow(const std::vector<std::string>& values) const;
    int getColumnIndex(const std::string& columnName) const;
    bool evaluateCondition(const std::vector<std::string>& row, const std::string& condition) const;
};

} // namespace soliddb 