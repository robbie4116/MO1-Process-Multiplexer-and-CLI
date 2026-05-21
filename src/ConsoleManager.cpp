#include "ConsoleManager.h"
#include <iostream>
#include <string>

void ConsoleManager::run() {
    std::string input;
    std::cout << "root:\\> ";
    while (std::getline(std::cin, input)) {
        if (input == "exit") break;
        std::cout << "Command not yet implemented: " << input << "\n";
        std::cout << "root:\\> ";
    }
}