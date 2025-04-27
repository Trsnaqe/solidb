#include "Table.h"
#include <sstream>
#include <algorithm>

namespace soliddb {

Table::Table(const std::string& name, const std::vector<std::pair<std::string, std::string>>& columns)
    : name_(name), columns_(columns) {
}

bool Table::insertRow(const std::vector<std::string>& values) {
    if (!validateRow(values)) {
        return false;
    }
    
    rows_.push_back(values);
    return true;
}

std::vector<std::vector<std::string>> Table::selectRows(
    const std::vector<std::string>& columns,
    const std::string& whereCondition) const {
    
    std::vector<std::vector<std::string>> result;
    
    bool returnAllColumns = columns.empty();
    
    std::vector<int> columnIndices;
    if (!returnAllColumns) {
        for (const auto& col : columns) {
            int idx = getColumnIndex(col);
            if (idx >= 0) {
                columnIndices.push_back(idx);
            }
        }
    } else {
        for (size_t i = 0; i < columns_.size(); i++) {
            columnIndices.push_back(static_cast<int>(i));
        }
    }
    
    for (const auto& row : rows_) {
        if (!whereCondition.empty() && !evaluateCondition(row, whereCondition)) {
            continue;
        }
        
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

const std::vector<std::pair<std::string, std::string>>& Table::getColumns() const {
    return columns_;
}

bool Table::validateRow(const std::vector<std::string>& values) const {
    if (values.size() != columns_.size()) {
        return false;
    }
    
    // For a real database, we would validate types here
    
    return true;
}

int Table::getColumnIndex(const std::string& columnName) const {
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].first == columnName) {
            return static_cast<int>(i);
        }
    }
    return -1; // Column not found
}

bool Table::evaluateCondition(const std::vector<std::string>& row, const std::string& condition) const {
    // Very simplified condition evaluation
    // Only supports conditions like "column=value"
    
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

std::string Table::serialize() const {
    std::stringstream ss;
    
    ss << name_ << std::endl;
    ss << columns_.size() << std::endl;
    
    for (const auto& [colName, colType] : columns_) {
        ss << colName << "," << colType << std::endl;
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
    
    std::vector<std::pair<std::string, std::string>> columns;
    for (int i = 0; i < columnCount; i++) {
        std::string columnDef;
        std::getline(ss, columnDef);
        
        size_t commaPos = columnDef.find(',');
        if (commaPos == std::string::npos) {
            return nullptr; // Invalid format
        }
        
        std::string colName = columnDef.substr(0, commaPos);
        std::string colType = columnDef.substr(commaPos + 1);
        columns.emplace_back(colName, colType);
    }
    
    // Create the table
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
        
        table->rows_.push_back(values);
    }
    
    return table;
}

} // namespace soliddb