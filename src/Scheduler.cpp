#include "scheduler.h"

#include <algorithm>
#include <cstdio>

Scheduler::Scheduler(int numCores) : numCores_(numCores) {
    cores_.resize(numCores_);
    for (int i = 0; i < numCores_; ++i) {
        cores_[i].id = i;
    }
}

void Scheduler::createProcesses(int count, int instructionsPerProcess) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (int i = 1; i <= count; ++i) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "p%02d", i);
        auto proc = std::make_shared<Process>(buf, i, instructionsPerProcess);
        allProcesses_.push_back(proc);
        readyQueue_.push(proc);
    }
}

void Scheduler::start() {
    schedulerThread_ = std::thread(&Scheduler::schedulerLoop, this);
    workerThreads_.reserve(numCores_);
    for (int i = 0; i < numCores_; ++i) {
        workerThreads_.emplace_back(&Scheduler::coreWorkerLoop, this, i);
    }
    cv_.notify_all(); // kick off scheduling immediately
}

void Scheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        shutdownRequested_ = true;
    }
    cv_.notify_all();
    if (schedulerThread_.joinable()) schedulerThread_.join();
    for (auto& w : workerThreads_) {
        if (w.joinable()) w.join();
    }
}

bool Scheduler::allFinished() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return finished_.size() == allProcesses_.size();
}

int Scheduler::totalProcessCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(allProcesses_.size());
}

SchedulerSnapshot Scheduler::getSnapshot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    SchedulerSnapshot snap;
    snap.numCores = numCores_;
    snap.coresUsed = 0;
    for (auto& c : cores_) {
        if (c.busy) snap.coresUsed++;
    }
    snap.allProcessesInOrder = allProcesses_;
    snap.finishedInOrder = finished_;
    return snap;
}

// Single dedicated thread. Implements FCFS: pops the ready queue in arrival
// order and hands work to any free core.
void Scheduler::schedulerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&] {
            if (shutdownRequested_) return true;
            bool hasFreeCore = std::any_of(cores_.begin(), cores_.end(),
                                            [](const CoreStatus& c) { return !c.busy; });
            return !readyQueue_.empty() && hasFreeCore;
        });

        if (shutdownRequested_) return; // stop dispatching new work; running cores finish on their own

        for (auto& core : cores_) {
            if (readyQueue_.empty()) break;
            if (core.busy) continue;
            auto proc = readyQueue_.front();
            readyQueue_.pop();
            proc->coreId = core.id;
            core.assigned = proc;
            core.busy = true;
        }
        lk.unlock();
        cv_.notify_all(); // wake the worker threads that just got an assignment
    }
}

// One persistent thread per CPU core. Waits for the scheduler to hand it a
// process, runs that process to completion, then waits for the next job.
void Scheduler::coreWorkerLoop(int coreId) {
    while (true) {
        std::shared_ptr<Process> proc;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cv_.wait(lk, [&] {
                return shutdownRequested_ || cores_[coreId].assigned != nullptr;
            });
            if (shutdownRequested_ && cores_[coreId].assigned == nullptr) return;
            proc = cores_[coreId].assigned;
        }

        proc->run(coreId); // executes all PRINT instructions, writes its own log file

        {
            std::lock_guard<std::mutex> lk(mutex_);
            cores_[coreId].assigned = nullptr;
            cores_[coreId].busy = false;
            finished_.push_back(proc);
        }
        cv_.notify_all(); // wake the scheduler: a core just freed up
    }
}
