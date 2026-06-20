#include "ConsoleManager.h"
#include "Config.h"
#include <iostream>
#include <iomanip>
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
    // Keep ConsoleManager lightweight: just show the header and return.
    // The main program owns the interactive loop so commands and scheduler
    // lifecycle are handled there.
    displayHeader();
    std::cout << "root:\\> ";
}

void ConsoleManager::printScreenLs(const Scheduler& scheduler) {
    SchedulerSnapshot snap = scheduler.getSnapshot();
    int coresAvailable = snap.numCores - snap.coresUsed;
    int utilization = (snap.numCores > 0) ? (snap.coresUsed * 100 / snap.numCores) : 0;

    std::cout << "\nCPU utilization: " << utilization << "%\n";
    std::cout << "Cores used: " << snap.coresUsed << "\n";
    std::cout << "Cores available: " << coresAvailable << "\n\n";
    std::cout << "--------------------------------\n";

    std::cout << "Running processes:\n";
    for (auto& proc : snap.allProcessesInOrder) {
        if (proc->state.load() == ProcState::RUNNING) {
            std::cout << std::left << std::setw(10) << proc->name
                       << " (" << proc->getStartTimestamp() << ")"
                       << "   Core: " << proc->coreId.load()
                       << "    " << proc->currentInstruction.load()
                       << " / " << proc->totalInstructions << "\n";
        }
    }

    std::cout << "\nFinished processes:\n";
    for (auto& proc : snap.finishedInOrder) {
        std::cout << std::left << std::setw(10) << proc->name
                   << " (" << proc->getStartTimestamp() << ")"
                   << "   Finished"
                   << "    " << proc->totalInstructions
                   << " / " << proc->totalInstructions << "\n";
    }
    std::cout << "--------------------------------\n\n";
}