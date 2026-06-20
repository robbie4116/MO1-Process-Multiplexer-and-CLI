// process.h - a single schedulable process and its PRINT-instruction log.
#pragma once

#include <atomic>
#include <mutex>
#include <string>

enum class ProcState { READY, RUNNING, FINISHED };

class Process {
public:
    Process(std::string name, int id, int totalInstructions);

    // Runs all PRINT instructions to completion on the given core
    // (FCFS is non-preemptive, so this owns the core until it's done),
    // writing each line to process_logs/<name>.txt as it goes.
    void run(int coreId);

    // Thread-safe accessor for the timestamp this process first started running.
    // Empty until the process has been dispatched to a core.
    std::string getStartTimestamp() const;

    const std::string name;
    const int id;
    const int totalInstructions;

    // Safe to read from any thread without locking.
    std::atomic<int> currentInstruction{0};
    std::atomic<int> coreId{-1};
    std::atomic<ProcState> state{ProcState::READY};

private:
    void setStartTimestamp(const std::string& ts);

    mutable std::mutex metaMutex_; // guards startTimestamp_ (not atomic-safe)
    std::string startTimestamp_;
};
