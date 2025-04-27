#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "core/Table.h"

namespace soliddb {
namespace core {

/**
 * Represents a database containing multiple tables
 */
class Database {
public:
    Database(const std::string& name);
    ~Database();

    bool createTable(const std::string& name, const std::vector<std::pair<std::string, std::string>>& columns);
    bool createTable(const std::string& name, const std::vector<ColumnDef>& columns);
    
    bool dropTable(const std::string& name);
    std::unique_ptr<Table>& getTable(const std::string& name);
    bool tableExists(const std::string& name) const;
    std::vector<std::string> getTableNames() const;
    
    bool loadFromFile();
    bool saveToFile() const;
    bool checkpoint() const;
    
    void logOperation(const std::string& operation);
    std::string getName() const;
    std::string getDataDir() const;
    
private:
    std::string name_;
    std::string dataDir_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;
    std::vector<std::string> wal_;
    int operationsSinceCheckpoint_ = 0;
    
    bool loadMetadata();
    bool saveMetadata() const;
    
    bool saveToTempFile(const std::string& path) const;
    std::string constructTablePath(const std::string& tableName) const;
};

} // namespace core
} // namespace soliddb 