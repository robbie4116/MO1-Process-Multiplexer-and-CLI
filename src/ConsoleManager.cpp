#include "ConsoleManager.h"
#include <iostream>
#include <string>

void ConsoleManager::displayHeader(){
    std::cout << "======================================================\n";

    std::cout << "  _____  _____  ____  _____  ______  _______     __\n";
    std::cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\   / /\n";
    std::cout << "| |    / (___ | |  | | |__) | |__  / (___  \\ \\ / / \n";
    std::cout << "| |     \\___ \\| |  | |  ___/|  __|  \\___ \\  \\   /  \n";
    std::cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   \n";
    std::cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   \n\n";

    std::cout << "======================================================\n\n";

    std::cout << "Welcome to the CSOPESY Emulator!\n\n";

    std::cout << "Developers: Cumti, Dulatre, Hong, Pineda\n";
    std::cout << "------------------------------------------------------\n\n";
}
void ConsoleManager::clearScreen(){
    #ifdef _WIN32 // if its a windows system
        system("cls");
    #else
        system("clear");
    #endif
}

void ConsoleManager::run() {
    std::string input;
    displayHeader();
    std::cout << "root:\\> ";
    
    while (std::getline(std::cin, input)) {
        if(input.empty()){
            displayHeader();
            std::cout << "root:\\> ";
            continue;
        }

        if (input == "exit") {
            clearScreen();
            break;
        } else if (input == "clear") {
            clearScreen();
            displayHeader();
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

        std::cout << "\nroot:\\> ";
    }
}