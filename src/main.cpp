// CSOPESY OS Emulator - Week 6 Homework
// FCFS CPU Scheduler with multi-threaded design (1 scheduler thread + 1 worker thread per core)
// and a PRINT instruction that logs to a per-process text file.
//
// Build:  make            (or: g++ -std=c++17 -O2 -pthread *.cpp -o csopesy_scheduler)
// Run:    ./csopesy_scheduler
//
// Test case implemented automatically on startup:
//   1. Creates 10 processes (p01..p10), each with 100 PRINT instructions.
//   2. The FCFS scheduler (4 cores) runs them to completion with no further
//      user input required.
//   3. Type "screen -ls" anytime to see running/finished processes.
//   4. Type "exit" to close the emulator.
//
// Each process writes its own log file to ./process_logs/<name>.txt
//
// See README.md for the file layout (config.h, process.*, scheduler.*,
// console_ui.*) and the threading model.

#include <filesystem>
#include <iostream>
#include <string>

#include "Config.h"
#include "ConsoleManager.h"
#include "Scheduler.h"
#include "Utils.h"

int main() {
    ConsoleManager console;

    std::filesystem::create_directories(config::OUTPUT_DIR);
    console.run();

    Scheduler scheduler(config::NUM_CORES);

    // Test case step 1: create 10 processes, each with 100 PRINT instructions,
    // upon the start of the emulator (FCFS arrival order p01..p10).
    scheduler.createProcesses(config::NUM_PROCESSES, config::INSTRUCTIONS_PER_PROCESS);

    std::cout << config::NUM_PROCESSES << " processes created (screen_01..screen_"
              << (config::NUM_PROCESSES < 10 ? "0" : "") << config::NUM_PROCESSES
              << "), each with " << config::INSTRUCTIONS_PER_PROCESS
              << " PRINT instructions.\n";
    std::cout << "Scheduler started. No further input is required for processes "
                 "to run to completion.\n\n";

    scheduler.start();

    // Main menu console loop
    std::string line;
    while (true) {
        std::cout << "root:\\> ";
        if (!std::getline(std::cin, line)) break;
        line = utils::trim(line);
        if (line.empty()) continue;

        if (line == "screen -ls") {
            console.printScreenLs(scheduler);
        } else if (line == "exit") {
            if (!scheduler.allFinished()) {
                std::cout << "Note: not all processes have finished yet; exiting anyway.\n";
            }
            break;
        } else {
            std::cout << "Unknown command. Available commands: screen -ls, exit\n";
        }
    }

    scheduler.shutdown(); // signals threads to stop and joins them

    std::cout << "\nAll " << config::NUM_PROCESSES << " process logs are in ./"
              << config::OUTPUT_DIR << "/  -- zip that folder for submission.\n";
    std::cout << "Emulator closed.\n";
    return 0;
}