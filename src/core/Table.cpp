#include "core/Table.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace soliddb {
namespace core {

Table::Table(const std::string& name, const std::vector<ColumnDef>& columns)
    : name_(name), columns_(columns) {
    
    uniqueIndexes_.resize(columns.size());
    
    int pkIndex = getPrimaryKeyColumnIndex();
    if (pkIndex >= 0) {
        // Add NOT_NULL constraint to primary key if not already set
        if (!columns_[pkIndex].isNotNull()) {
            columns_[pkIndex].constraints |= static_cast<int>(ColumnConstraint::NOT_NULL);
        }
    }
}

Table::Table(const std::string& name, const std::vector<std::pair<std::string, std::string>>& columns)
    : name_(name) {
    
    // Convert simple columns to ColumnDef objects (with no constraints)
    columns_.reserve(columns.size());
    for (const auto& [colName, colType] : columns) {
        columns_.emplace_back(colName, colType);
    }
    
    // Initialize empty unique indexes
    uniqueIndexes_.resize(columns.size());
}

bool Table::insertRow(const std::vector<std::string>& values) {
    if (!validateRow(values)) {
        return false;
    }
    
    // Check constraints (PK, UNIQUE, NOT NULL)
    if (!checkConstraints(values)) {
        return false;
    }
    
    // Add row
    rows_.push_back(values);
    
    // Update indexes
    size_t rowIndex = rows_.size() - 1;
    
    // Update primary key index if there is one
    int pkIndex = getPrimaryKeyColumnIndex();
    if (pkIndex >= 0) {
        primaryKeyIndex_[values[pkIndex]] = rowIndex;
    }
    
    // Update unique indexes
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].requiresUniqueValue()) {
            uniqueIndexes_[i].insert(values[i]);
        }
    }
    
    return true;
}

std::vector<std::vector<std::string>> Table::selectRows(
    const std::vector<std::string>& columns,
    const std::string& whereCondition) const {
    
    std::vector<std::vector<std::string>> result;
    
    // If no columns specified, return all columns
    bool returnAllColumns = columns.empty();
    
    // Determine column indices to return
    std::vector<int> columnIndices;
    if (!returnAllColumns) {
        for (const auto& col : columns) {
            int idx = getColumnIndex(col);
            if (idx >= 0) {
                columnIndices.push_back(idx);
            }
        }
    } else {
        // Return all columns
        for (size_t i = 0; i < columns_.size(); i++) {
            columnIndices.push_back(static_cast<int>(i));
        }
    }
    
    // Process each row
    for (const auto& row : rows_) {
        // Skip rows that don't satisfy the condition
        if (!whereCondition.empty() && !evaluateCondition(row, whereCondition)) {
            continue;
        }
        
        // Add matching row (with selected columns) to result
        std::vector<std::string> resultRow;
        for (int idx : columnIndices) {
            resultRow.push_back(row[idx]);
        }
        result.push_back(resultRow);
    }
    
    return result;
}

std::string Table::getName() const {
    return name_;
}

std::vector<std::pair<std::string, std::string>> Table::getColumnsAsNameTypePairs() const {
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(columns_.size());
    
    for (const auto& col : columns_) {
        result.emplace_back(col.name, col.type);
    }
    
    return result;
}

const std::vector<ColumnDef>& Table::getColumns() const {
    return columns_;
}

size_t Table::getRowCount() const {
    return rows_.size();
}

bool Table::validateRow(const std::vector<std::string>& values) const {
    if (values.size() != columns_.size()) {
        std::cout << "Error: Expected " << columns_.size() << " values, got " << values.size() << std::endl;
        return false;
    }
    
    // TODO: we would validate types here
    
    return true;
}

bool Table::checkConstraints(const std::vector<std::string>& values) {
    //  NOT NULL constraints
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].isNotNull() && values[i].empty()) {
            std::cout << "Error: Column '" << columns_[i].name << "' cannot be NULL" << std::endl;
            return false;
        }
    }
    
    //  PRIMARY KEY constraint
    int pkIndex = getPrimaryKeyColumnIndex();
    if (pkIndex >= 0) {
        const std::string& pkValue = values[pkIndex];
        if (primaryKeyIndex_.find(pkValue) != primaryKeyIndex_.end()) {
            std::cout << "Error: Duplicate primary key value '" << pkValue << "'" << std::endl;
            return false;
        }
    }
    
    //  UNIQUE constraints
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].isUnique() && !columns_[i].isPrimaryKey()) {
            const std::string& uniqueValue = values[i];
            if (!uniqueValue.empty() && uniqueIndexes_[i].find(uniqueValue) != uniqueIndexes_[i].end()) {
                std::cout << "Error: Duplicate value '" << uniqueValue << "' in unique column '" 
                          << columns_[i].name << "'" << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

int Table::getColumnIndex(const std::string& columnName) const {
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].name == columnName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Table::evaluateCondition(const std::vector<std::string>& row, const std::string& condition) const {
    // only supports conditions like "column=value"
    
    size_t pos = condition.find('=');
    if (pos == std::string::npos) {
        return true; 
    }
    
    std::string columnName = condition.substr(0, pos);
    std::string value = condition.substr(pos + 1);
    
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    
    int colIndex = getColumnIndex(columnName);
    if (colIndex < 0 || colIndex >= static_cast<int>(row.size())) {
        return false;
    }
    
    return row[colIndex] == value;
}

int Table::getPrimaryKeyColumnIndex() const {
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].isPrimaryKey()) {
            return static_cast<int>(i);
        }
    }
    return -1; 
}

std::string Table::serialize() const {
    std::stringstream ss;
    
    ss << name_ << std::endl;
    ss << columns_.size() << std::endl;
    
    for (const auto& col : columns_) {
        ss << col.name << "," << col.type << "," << col.constraints << std::endl;
    }
    
    ss << rows_.size() << std::endl;
    for (const auto& row : rows_) {
        for (size_t i = 0; i < row.size(); i++) {
            ss << row[i];
            if (i < row.size() - 1) {
                ss << ",";
            }
        }
        ss << std::endl;
    }
    
    return ss.str();
}

std::unique_ptr<Table> Table::deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string line;
    
    std::string tableName;
    std::getline(ss, tableName);
    
    int columnCount;
    ss >> columnCount;
    ss.ignore(); 
    
    std::vector<ColumnDef> columns;
    for (int i = 0; i < columnCount; i++) {
        std::string columnDef;
        std::getline(ss, columnDef);
        
        std::stringstream colStream(columnDef);
        std::string colName, colType;
        int constraints = 0;
        
        std::getline(colStream, colName, ',');
        std::getline(colStream, colType, ',');
        
        std::string constraintsStr;
        if (std::getline(colStream, constraintsStr, ',')) {
            try {
                constraints = std::stoi(constraintsStr);
            } catch (...) {
                constraints = 0;
            }
        }
        
        columns.emplace_back(colName, colType, constraints);
    }
    
    auto table = std::make_unique<Table>(tableName, columns);
    
    int rowCount;
    ss >> rowCount;
    ss.ignore(); 
    
    for (int i = 0; i < rowCount; i++) {
        std::string rowData;
        std::getline(ss, rowData);
        
        std::vector<std::string> values;
        std::stringstream rowStream(rowData);
        std::string value;
        
        while (std::getline(rowStream, value, ',')) {
            values.push_back(value);
        }
        
        table->insertRow(values);
    }
    
    return table;
}

} // namespace core
} // namespace soliddb 