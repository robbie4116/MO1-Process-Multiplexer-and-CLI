// include/ConsoleManager.h
#pragma once
#include <memory>
#include "Scheduler.h"
#include "Config.h"

class ConsoleManager {
public:
    void run();  // main interactive loop

private:
    // Commands
    void handleInitialize();
    void handleScreen(const std::string& args);
    void handleSchedulerStart();
    void handleSchedulerStop();
    void handleReportUtil();
    void handleScreenLs();

    // Screen sub-shell
    void enterProcessScreen(const std::shared_ptr<Process>& proc);

    // Rendering helpers
    void displayHeader();
    void clearScreen();
    void printScreenLsOutput(const SchedulerSnapshot& snap, std::ostream& out);
    void printPrompt();

    bool             initialized_ = false;
    Config           config_;
    std::unique_ptr<Scheduler> scheduler_;
};