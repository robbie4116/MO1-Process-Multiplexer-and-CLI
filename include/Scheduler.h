// include/Scheduler.h
#pragma once
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include "Process.h"
#include "Config.h"

struct CoreStatus {
    int id       = -1;
    bool busy    = false;
    std::shared_ptr<Process> assigned;
};

struct SchedulerSnapshot {
    int numCores  = 0;
    int coresUsed = 0;
    uint64_t cpuCycles = 0;
    std::vector<std::shared_ptr<Process>> allProcessesInOrder;
    std::vector<std::shared_ptr<Process>> finishedInOrder;
};

class Scheduler {
public:
    explicit Scheduler(const Config& cfg);
    ~Scheduler();

    // Add a manually created process (screen -s).
    void addProcess(std::shared_ptr<Process> proc);

    // Start the scheduler thread(s).
    void start();
    void shutdown();

    // scheduler-start / scheduler-stop
    void startBatchGeneration();
    void stopBatchGeneration();
    bool isBatchRunning() const;

    // Find a process by name (for screen -r).
    std::shared_ptr<Process> findProcess(const std::string& name) const;

    bool allFinished() const;
    SchedulerSnapshot getSnapshot() const;

    // Next auto-generated process index (p01, p02, ...).
    int nextProcessIndex() const;

private:
    void mainLoop();   // one thread drives everything: tick, dispatch, preempt
    void dispatchFCFS();
    void dispatchRR();
    void generateBatchProcess();
    std::string makeProcessName(int idx);

    Config cfg_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::queue<std::shared_ptr<Process>>  readyQueue_;
    std::vector<CoreStatus>               cores_;
    std::vector<std::shared_ptr<Process>> allProcesses_;
    std::vector<std::shared_ptr<Process>> finished_;

    // RR: quantum remaining per core
    std::vector<uint32_t> quantumLeft_;
    std::vector<uint32_t> coreDelayCounter_;
    std::atomic<uint64_t> cpuCycles_{0};
    std::atomic<bool>     batchRunning_{false};
    std::atomic<int>      nextProcIdx_{1};

    bool shutdownRequested_ = false;
    std::thread mainThread_;
};