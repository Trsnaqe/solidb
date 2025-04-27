#include "Database.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <sstream>

namespace fs = std::filesystem;
namespace soliddb {

Database::Database(const std::string& name) : name_(name) {
    // Create the database directory if it doesn't exist
    fs::create_directories(name);
}

bool Database::createTable(const std::string& tableName, 
                          const std::vector<std::pair<std::string, std::string>>& columns) {
    if (tables_.find(tableName) != tables_.end()) {
        return false; 
    }
    
    tables_[tableName] = std::make_unique<Table>(tableName, columns);
    return true;
}

bool Database::insert(const std::string& tableName, const std::vector<std::string>& values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        return false; 
    }
    
    return it->second->insertRow(values);
}

std::vector<std::vector<std::string>> Database::select(
    const std::string& tableName, 
    const std::vector<std::string>& columns,
    const std::string& whereCondition) {
    
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        return {}; 
    }
    
    return it->second->selectRows(columns, whereCondition);
}

std::string Database::getName() const {
    return name_;
}

bool Database::saveToFile() const {
    try {
        fs::create_directories(name_);
        
        std::ofstream metaFile(name_ + "/metadata.db");
        if (!metaFile) {
            return false;
        }
        
        metaFile << tables_.size() << std::endl;
        for (const auto& [tableName, _] : tables_) {
            metaFile << tableName << std::endl;
        }
        
        for (const auto& [tableName, table] : tables_) {
            std::ofstream tableFile(name_ + "/" + tableName + ".tbl");
            if (!tableFile) {
                return false;
            }
            tableFile << table->serialize();
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::unique_ptr<Database> Database::loadFromFile(const std::string& name) {
    try {
        if (!fs::exists(name)) {
            return nullptr; 
        }
        
        auto db = std::make_unique<Database>(name);
        
        std::ifstream metaFile(name + "/metadata.db");
        if (!metaFile) {
            return nullptr;
        }
        
        int tableCount;
        metaFile >> tableCount;
        metaFile.ignore(); 
        
        for (int i = 0; i < tableCount; i++) {
            std::string tableName;
            std::getline(metaFile, tableName);
            
            std::ifstream tableFile(name + "/" + tableName + ".tbl");
            if (!tableFile) {
                continue;
            }
            
            std::stringstream buffer;
            buffer << tableFile.rdbuf();
            auto table = Table::deserialize(buffer.str());
            
            if (table) {
                db->tables_[tableName] = std::move(table);
            }
        }
        
        return db;
    } catch (const std::exception&) {
        return nullptr;
    }
}

} // namespace soliddb 