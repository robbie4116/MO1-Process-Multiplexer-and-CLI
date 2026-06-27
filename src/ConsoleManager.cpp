// src/ConsoleManager.cpp
#include "ConsoleManager.h"
#include "ConfigParser.h"
#include "Instruction.h"
#include "Utils.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

void ConsoleManager::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ConsoleManager::displayHeader() {
    std::cout << "======================================================\n";
    std::cout << "  _____  _____  ____  _____  ______  _______     __\n";
    std::cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____|\\   / /\n";
    std::cout << "| |    / (___ | |  | | |__) | |__  / (___  \\ \\ / / \n";
    std::cout << "| |     \\___ \\| |  | |  ___/|  __|  \\___ \\  \\   /  \n";
    std::cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   \n";
    std::cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   \n\n";
    std::cout << "======================================================\n\n";
    std::cout << "Welcome to the CSOPESY Emulator!\n\n";
    std::cout << "Developers: Cumti, Dulatre, Hong, Pineda\n";
    std::cout << "------------------------------------------------------\n\n";
}

void ConsoleManager::printPrompt() {
    std::cout << "root:\\> ";
    std::cout.flush();
}

// ── Initialize ────────────────────────────────────────────────────────────────

void ConsoleManager::handleInitialize() {
    if (!ConfigParser::parse("config.txt", config_)) {
        std::cout << "Error: failed to read config.txt.\n";
        return;
    }
    scheduler_ = std::make_unique<Scheduler>(config_);
    scheduler_->start();
    initialized_ = true;
    std::cout << "Initialized successfully.\n";
}

// ── screen -ls / report-util shared render ────────────────────────────────────

void ConsoleManager::printScreenLsOutput(const SchedulerSnapshot& snap,
                                          std::ostream& out) {
    int avail       = snap.numCores - snap.coresUsed;
    int utilization = snap.numCores > 0
                        ? (snap.coresUsed * 100 / snap.numCores)
                        : 0;

    out << "\nCPU utilization: " << utilization << "%\n";
    out << "Cores used: "      << snap.coresUsed << "\n";
    out << "Cores available: " << avail          << "\n\n";
    out << "--------------------------------------\n";

    out << "Running processes:\n";
    for (const auto& p : snap.runningProcesses) {
        out << std::left << std::setw(12) << p.name
            << " (" << p.startTimestamp << ")"
            << "   Core: " << p.coreId
            << "    "      << p.currentInstruction
            << " / "       << p.totalInstructions << "\n";
    }

    out << "\nFinished processes:\n";
    for (const auto& p : snap.finishedProcesses) {
        out << std::left << std::setw(12) << p.name
            << " (" << p.startTimestamp << ")"
            << "   Finished"
            << "    " << p.totalInstructions
            << " / "  << p.totalInstructions << "\n";
    }
    out << "--------------------------------------\n\n";
}

void ConsoleManager::handleScreenLs() {
    auto snap = scheduler_->getSnapshot();
    printScreenLsOutput(snap, std::cout);
}

void ConsoleManager::handleReportUtil() {
    auto snap = scheduler_->getSnapshot();
    std::ofstream file("csopesy-log.txt");
    if (!file.is_open()) {
        std::cout << "Error: could not create csopesy-log.txt\n";
        return;
    }
    printScreenLsOutput(snap, file);
    file.close();
    std::cout << "Report generated at csopesy-log.txt\n";
}

// ── screen -s / screen -r ─────────────────────────────────────────────────────

void ConsoleManager::enterProcessScreen(const std::shared_ptr<Process>& proc) {
    clearScreen();
    std::string cmd;
    while (true) {
        std::cout << "root:\\> ";
        std::cout.flush();
        if (!std::getline(std::cin, cmd)) break;
        cmd = utils::trim(cmd);

        if (cmd == "exit") {
            clearScreen();
            displayHeader();
            break;
        } else if (cmd == "process-smi") {
            std::cout << "\nProcess name: " << proc->name << "\n";
            std::cout << "ID: "            << proc->id   << "\n";
            std::cout << "Logs:\n";
            for (auto& line : proc->getLogs())
                std::cout << line << "\n";
            std::cout << "\n";

            auto st = proc->state.load();
            if (st == ProcState::FINISHED) {
                std::cout << "Finished!\n\n";
            } else {
                std::cout << "Current instruction line: "
                          << proc->currentInstruction.load() << "\n";
                std::cout << "Lines of code: "
                          << proc->totalInstructions << "\n\n";
            }
        } else {
            std::cout << "Unknown command. Use 'process-smi' or 'exit'.\n";
        }
    }
}

void ConsoleManager::handleScreen(const std::string& args) {
    std::istringstream iss(args);
    std::string flag, pname;
    iss >> flag;

    if (flag == "-s") {
        iss >> pname;
        if (pname.empty()) { std::cout << "Usage: screen -s <name>\n"; return; }
        if (scheduler_->findProcess(pname)) {
            std::cout << "Process '" << pname << "' already exists.\n"; return;
        }
        try {
            int idx = scheduler_->allocateProcessId();
            auto instrs = generateRandomInstructions(
                pname, config_.minIns, config_.maxIns);
            auto proc =
                std::make_shared<Process>(pname, idx, std::move(instrs));
            scheduler_->addProcess(proc);
            enterProcessScreen(proc);
        } catch (const std::exception& error) {
            std::cout << "Error: could not create process: "
                      << error.what() << "\n";
        }

    } else if (flag == "-r") {
        iss >> pname;
        if (pname.empty()) { std::cout << "Usage: screen -r <name>\n"; return; }
        auto proc = scheduler_->findProcess(pname);
        if (!proc || proc->state.load() == ProcState::FINISHED) {
            std::cout << "Process " << pname << " not found.\n"; return;
        }
        enterProcessScreen(proc);

    } else if (flag == "-ls") {
        handleScreenLs();

    } else {
        std::cout << "Usage: screen [-s <name>] [-r <name>] [-ls]\n";
    }
}

// ── Scheduler start/stop ──────────────────────────────────────────────────────

void ConsoleManager::handleSchedulerStart() {
    if (scheduler_->isBatchRunning()) {
        std::cout << "Scheduler is already running.\n"; return;
    }
    scheduler_->startBatchGeneration();
    std::cout << "Scheduler started.\n";
}

void ConsoleManager::handleSchedulerStop() {
    scheduler_->stopBatchGeneration();
    std::cout << "Scheduler stopped.\n";
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void ConsoleManager::run() {
    clearScreen();
    displayHeader();

    std::string line;
    while (true) {
        printPrompt();
        if (!std::getline(std::cin, line)) break;
        line = utils::trim(line);
        if (line.empty()) continue;

        // Always allow exit
        if (line == "exit") {
            if (scheduler_) scheduler_->shutdown();
            std::cout << "Goodbye.\n";
            break;
        }

        // Before initialize, nothing else is recognized
        if (!initialized_) {
            if (line == "initialize") {
                handleInitialize();
            } else {
                std::cout << "Please run 'initialize' first.\n";
            }
            continue;
        }

        // ── Recognized commands ────────────────────────────────────────────
        if (line == "initialize") {
            std::cout << "Already initialized.\n";
        } else if (line == "screen-ls") {
            handleScreenLs();
        } else if (line.rfind("screen", 0) == 0) {
            std::string args = line.size() > 7 ? line.substr(7) : "";
            handleScreen(utils::trim(args));
        } else if (line == "scheduler-test") {
            handleSchedulerStart();
        } else if (line == "scheduler-start") {
            handleSchedulerStart();
        } else if (line == "scheduler-stop") {
            handleSchedulerStop();
        } else if (line == "report-util") {
            handleReportUtil();
        } else {
            std::cout << "Unknown command: " << line << "\n";
        }
    }
}
