#include "ConsoleManager.h"
#include <iostream>
#include <string>

void ConsoleManager::displayHeader(){}
void ConsoleManager::clearScreen(){}

void ConsoleManager::run() {
    std::string input;
    std::cout << "root:\\> ";
    
    while (std::getline(std::cin, input)) {
        if(input.empty()){
            std::cout << "root:\\> ";
            continue;
        }

        if (input == "exit") {
            break;
        } else if (
            input == "initialize" || 
            input == "screen" || 
            input == "scheduler-start" || 
            input == "scheduler-stop" || 
            input == "report-util") {
            std::cout << input << " command recognized. Doing something.\n";
        } else {
            std::cout << "Command not yet implemented: " << input << "\n";
        }
        
        std::cout << "root:\\> ";
    }
}