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
#include "SimProcess.h"
#include "Config.h"

struct CoreStatus {
    int id       = -1;
    bool busy    = false;
    std::shared_ptr<Process> assigned;
};

struct ProcessStatusSnapshot {
    std::string name;
    int id = 0;
    int coreId = -1;
    uint64_t currentInstruction = 0;
    uint64_t totalInstructions = 0;
    std::string startTimestamp;
};

struct SchedulerSnapshot {
    int numCores  = 0;
    int coresUsed = 0;
    uint64_t cpuCycles = 0;
    std::vector<ProcessStatusSnapshot> runningProcesses;
    std::vector<ProcessStatusSnapshot> finishedProcesses;
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

    // Allocate one globally unique process ID.
    int allocateProcessId();

private:
    void mainLoop();   // one thread drives everything: tick, dispatch, preempt
    void dispatchFCFS();
    void dispatchRR();
    void generateBatchProcess();
    void wakeSleepingProcesses(uint64_t tick);
    void releaseCore(int coreIndex);
    void dispatchFreeCores();
    std::string makeProcessName(int idx);

    Config cfg_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::queue<std::shared_ptr<Process>>  readyQueue_;
    std::vector<std::shared_ptr<Process>> sleepingProcesses_;
    std::vector<CoreStatus>               cores_;
    std::vector<std::shared_ptr<Process>> allProcesses_;
    std::vector<std::shared_ptr<Process>> finished_;

    // RR: quantum remaining per core
    std::vector<uint32_t> quantumLeft_;
    std::atomic<uint64_t> cpuCycles_{0};
    std::atomic<bool>     batchRunning_{false};
    std::atomic<int>      nextProcessId_{1};
    int                   nextBatchNameIndex_ = 1;
    uint64_t              nextBatchGenerationTick_ = 0;

    bool shutdownRequested_ = false;
    std::thread mainThread_;
};
