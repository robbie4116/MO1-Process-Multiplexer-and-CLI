#pragma once

#include "Scheduler.h"

class ConsoleManager {
  public:
    void run();
    void displayHeader();
    void clearScreen();
    // Renders the `screen -ls` view (CPU utilization, cores used/available,
// running/finished process lists) from a live snapshot of the scheduler.
    void printScreenLs(const Scheduler& scheduler); 
};