// scheduler.h - FCFS CPU scheduler.
//
// Threading model:
//   - 1 scheduler thread owns the ready queue and, whenever a core is free,
//     hands it the next process in arrival order.
//   - 1 worker thread per core waits for an assignment, runs that process
//     to completion (FCFS = non-preemptive), then reports back.
#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Process.h"

struct CoreStatus {
    int id = -1;
    bool busy = false;
    std::shared_ptr<Process> assigned; // nullptr if idle
};

// Read-only snapshot used by the UI layer to render `screen -ls`.
struct SchedulerSnapshot {
    int numCores = 0;
    int coresUsed = 0;
    std::vector<std::shared_ptr<Process>> allProcessesInOrder; // FCFS arrival order
    std::vector<std::shared_ptr<Process>> finishedInOrder;     // completion order
};

class Scheduler {
public:
    explicit Scheduler(int numCores);

    // Creates `count` processes (named p01, p02, ...), each with
    // `instructionsPerProcess` PRINT instructions, and enqueues them in
    // FCFS order. Call before start().
    void createProcesses(int count, int instructionsPerProcess);

    // Launches the scheduler thread and one worker thread per core.
    void start();

    // Signals shutdown and joins every thread. Safe to call once, after start().
    void shutdown();

    bool allFinished() const;
    int totalProcessCount() const;

    SchedulerSnapshot getSnapshot() const;

private:
    void schedulerLoop();
    void coreWorkerLoop(int coreId);

    int numCores_;

    mutable std::mutex mutex_; // guards everything below
    std::condition_variable cv_;
    std::queue<std::shared_ptr<Process>> readyQueue_;
    std::vector<CoreStatus> cores_;
    std::vector<std::shared_ptr<Process>> allProcesses_;
    std::vector<std::shared_ptr<Process>> finished_;
    bool shutdownRequested_ = false;

    std::thread schedulerThread_;
    std::vector<std::thread> workerThreads_;
};
