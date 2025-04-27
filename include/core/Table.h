#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace soliddb {
namespace core {

/**
 * Column constraint types
 */
enum class ColumnConstraint {
    NONE = 0,
    PRIMARY_KEY = 1,
    UNIQUE = 2,
    NOT_NULL = 4
};

/**
 * Column definition with name, type and constraints
 */
struct ColumnDef {
    std::string name;
    std::string type;
    int constraints;  // Bitmask of ColumnConstraint values

    ColumnDef(const std::string& n, const std::string& t, int c = 0)
        : name(n), type(t), constraints(c) {}

    bool isPrimaryKey() const { return (constraints & static_cast<int>(ColumnConstraint::PRIMARY_KEY)) != 0; }
    bool isUnique() const { return (constraints & static_cast<int>(ColumnConstraint::UNIQUE)) != 0; }
    bool isNotNull() const { return (constraints & static_cast<int>(ColumnConstraint::NOT_NULL)) != 0; }
    bool requiresUniqueValue() const { return isPrimaryKey() || isUnique(); }
};

/**
 * Represents a table in the database
 */
class Table {
public:
    /**
     * Create a new table with the given name and column definitions
     */
    Table(const std::string& name, const std::vector<ColumnDef>& columns);

    /**
     * Create a table with basic column definitions (backwards compatibility)
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
    std::vector<std::pair<std::string, std::string>> getColumnsAsNameTypePairs() const;

    /**
     * Get column definitions with constraints
     */
    const std::vector<ColumnDef>& getColumns() const;

    /**
     * Get the number of rows in the table
     */
    size_t getRowCount() const;

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
    std::vector<ColumnDef> columns_;
    std::vector<std::vector<std::string>> rows_;
    
    // Index for primary key lookup
    std::unordered_map<std::string, size_t> primaryKeyIndex_;
    
    // Indexes for unique columns
    std::vector<std::unordered_set<std::string>> uniqueIndexes_;

    // Helper methods
    bool validateRow(const std::vector<std::string>& values) const;
    bool checkConstraints(const std::vector<std::string>& values);
    int getColumnIndex(const std::string& columnName) const;
    bool evaluateCondition(const std::vector<std::string>& row, const std::string& condition) const;
    int getPrimaryKeyColumnIndex() const;
};

} // namespace core
} // namespace soliddb 