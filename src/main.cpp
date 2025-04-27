#include "SolidDB.h"
#include <iostream>
#include <string>
#include <memory>

using namespace soliddb;

int main() {
    std::shared_ptr<core::Database> currentDatabase;
    parser::CommandParser parser;
    std::string input;
    
    std::cout << "Welcome to SolidDB v" << VERSION << "!\n";
    std::cout << "Type HELP for a list of commands or EXIT to quit.\n";
    
    while (true) {
        if (currentDatabase) {
            std::cout << currentDatabase->getName() << "> ";
        } else {
            std::cout << "SolidDB> ";
        }
        
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        bool continueRunning = parser.executeCommand(input, currentDatabase);
        if (!continueRunning) {
            break;
        }
    }
    
    if (currentDatabase) {
        currentDatabase->saveToFile();
    }
    
    std::cout << "Goodbye!\n";
    return 0;
}

