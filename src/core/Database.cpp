#include "core/Database.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace fs = std::filesystem;
namespace soliddb {
namespace core {

Database::Database(const std::string& name) : name_(name) {
    fs::create_directories(name);
}

bool Database::createTable(const std::string& tableName, 
                          const std::vector<core::ColumnDef>& columns) {
    if (tables_.find(tableName) != tables_.end()) {
        std::cout << "Error: Table '" << tableName << "' already exists." << std::endl;
        return false;
    }
    
    tables_[tableName] = std::make_unique<Table>(tableName, columns);
    std::cout << "Table '" << tableName << "' created with constraints." << std::endl;
    return true;
}

bool Database::createTable(const std::string& tableName, 
                         const std::vector<std::pair<std::string, std::string>>& columns) {
    if (tables_.find(tableName) != tables_.end()) {
        std::cout << "Error: Table '" << tableName << "' already exists." << std::endl;
        return false;
    }
    
    tables_[tableName] = std::make_unique<Table>(tableName, columns);
    std::cout << "Table '" << tableName << "' created." << std::endl;
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

std::vector<std::string> Database::getTableNames() const {
    std::vector<std::string> names;
    names.reserve(tables_.size());
    
    for (const auto& [name, _] : tables_) {
        names.push_back(name);
    }
    
    return names;
}

bool Database::tableExists(const std::string& tableName) const {
    return tables_.find(tableName) != tables_.end();
}

bool Database::saveToFile() const {
    try {
        fs::create_directories(name_);
        
              // initially save to temporary files & tables
        std::string tempMetaFile = name_ + "/metadata.db.tmp";
        
        std::ofstream metaFile(tempMetaFile);
        if (!metaFile) {
            std::cerr << "Error: Failed to open metadata file for writing" << std::endl;
            return false;
        }
        
        metaFile << tables_.size() << std::endl;
        for (const auto& [tableName, _] : tables_) {
            metaFile << tableName << std::endl;
        }
        metaFile.close();
        
        bool allTablesSuccess = true;
        for (const auto& [tableName, table] : tables_) {
            std::string tempTableFile = name_ + "/" + tableName + ".tbl.tmp";
            std::ofstream tableFile(tempTableFile);
            if (!tableFile) {
                std::cerr << "Error: Failed to open table file " << tableName << " for writing" << std::endl;
                allTablesSuccess = false;
                break;
            }
            tableFile << table->serialize();
            tableFile.close();
        }
        
        if (allTablesSuccess) {
            fs::rename(tempMetaFile, name_ + "/metadata.db");
            
            for (const auto& [tableName, _] : tables_) {
                std::string tempTableFile = name_ + "/" + tableName + ".tbl.tmp";
                std::string finalTableFile = name_ + "/" + tableName + ".tbl";
                fs::rename(tempTableFile, finalTableFile);
            }
            return true;
        } else {
            if (fs::exists(tempMetaFile)) {
                fs::remove(tempMetaFile);
            }
            
            for (const auto& [tableName, _] : tables_) {
                std::string tempTableFile = name_ + "/" + tableName + ".tbl.tmp";
                if (fs::exists(tempTableFile)) {
                    fs::remove(tempTableFile);
                }
            }
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving database: " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<Database> Database::loadFromFile(const std::string& name) {
    try {
        if (!fs::exists(name)) {
            std::cerr << "Error: Database directory does not exist: " << name << std::endl;
            return nullptr;
        }
        
        std::string metadataPath = name + "/metadata.db";
        if (!fs::exists(metadataPath)) {
            std::cerr << "Error: Metadata file not found: " << metadataPath << std::endl;
            return nullptr;
        }
        
        auto db = std::make_unique<Database>(name);
        
        std::ifstream metaFile(metadataPath);
        if (!metaFile) {
            std::cerr << "Error: Failed to open metadata file: " << metadataPath << std::endl;
            return nullptr;
        }
        
        int tableCount;
        metaFile >> tableCount;
        metaFile.ignore();
        
        for (int i = 0; i < tableCount; i++) {
            std::string tableName;
            std::getline(metaFile, tableName);
            
            std::string tableFilePath = name + "/" + tableName + ".tbl";
            if (!fs::exists(tableFilePath)) {
                std::cerr << "Warning: Table file not found: " << tableFilePath << std::endl;
                continue;
            }
            
            std::ifstream tableFile(tableFilePath);
            if (!tableFile) {
                std::cerr << "Warning: Failed to open table file: " << tableFilePath << std::endl;
                continue;
            }
            
            std::stringstream buffer;
            buffer << tableFile.rdbuf();
            auto table = Table::deserialize(buffer.str());
            
            if (table) {
                db->tables_[tableName] = std::move(table);
                std::cout << "Loaded table: " << tableName << std::endl;
            } else {
                std::cerr << "Warning: Failed to deserialize table: " << tableName << std::endl;
            }
        }
        
        return db;
    } catch (const std::exception& e) {
        std::cerr << "Error loading database: " << e.what() << std::endl;
        return nullptr;
    }
}

void Database::logOperation(const std::string& operation) {
    wal_.push_back(operation);
    
    operationsSinceCheckpoint_++;
    
    try {
        std::string logFilePath = name_ + "/transactions.log";
        
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile) {
            logFile << operation << std::endl;
        } else {
            std::cerr << "Warning: Failed to write to transaction log file" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error logging operation: " << e.what() << std::endl;
    }
    
    if (operationsSinceCheckpoint_ >= 5) {
        checkpoint();
    }
}

bool Database::checkpoint() const {
    std::cout << "Performing checkpoint..." << std::endl;
    
    bool success = saveToFile();
    
    if (success) {
        // reset the operations counter 
        Database* nonConstThis = const_cast<Database*>(this);
        nonConstThis->operationsSinceCheckpoint_ = 0;
        
        // clear the WAL after  checkpoint
        nonConstThis->wal_.clear();
        
        std::cout << "Checkpoint completed successfully" << std::endl;
    } else {
        std::cerr << "Checkpoint failed" << std::endl;
    }
    
    return success;
}

} // namespace core
} // namespace soliddb 