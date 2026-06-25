// src/Scheduler.cpp
#include "Scheduler.h"
#include "Instruction.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <iomanip>

Scheduler::Scheduler(const Config& cfg) : cfg_(cfg) {
    cores_.resize(cfg_.numCpu);
    quantumLeft_.resize(cfg_.numCpu, cfg_.quantumCycles);
    coreDelayCounter_.resize(cfg_.numCpu, 0); 
    for (int i = 0; i < cfg_.numCpu; ++i) cores_[i].id = i;
}

Scheduler::~Scheduler() { shutdown(); }

std::string Scheduler::makeProcessName(int idx) {
    std::ostringstream oss;
    oss << "p" << std::setfill('0') << std::setw(2) << idx;
    return oss.str();
}

int Scheduler::nextProcessIndex() const {
    return nextProcIdx_.load();
}

void Scheduler::addProcess(std::shared_ptr<Process> proc) {
    std::lock_guard<std::mutex> lk(mutex_);
    allProcesses_.push_back(proc);
    readyQueue_.push(proc);
    cv_.notify_all();
}

std::shared_ptr<Process> Scheduler::findProcess(const std::string& name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& p : allProcesses_)
        if (p->name == name) return p;
    return nullptr;
}

bool Scheduler::allFinished() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return !allProcesses_.empty() && finished_.size() == allProcesses_.size();
}

SchedulerSnapshot Scheduler::getSnapshot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    SchedulerSnapshot snap;
    snap.numCores  = cfg_.numCpu;
    snap.cpuCycles = cpuCycles_.load();
    for (auto& c : cores_) if (c.busy) snap.coresUsed++;
    snap.allProcessesInOrder = allProcesses_;
    snap.finishedInOrder     = finished_;
    return snap;
}

void Scheduler::startBatchGeneration() { batchRunning_.store(true); }
void Scheduler::stopBatchGeneration()  { batchRunning_.store(false); }
bool Scheduler::isBatchRunning() const { return batchRunning_.load(); }

void Scheduler::start() {
    mainThread_ = std::thread(&Scheduler::mainLoop, this);
}

void Scheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        shutdownRequested_ = true;
    }
    cv_.notify_all();
    if (mainThread_.joinable()) mainThread_.join();
}

// ── Batch generation ──────────────────────────────────────────────────────────
void Scheduler::generateBatchProcess() {
    int idx = nextProcIdx_.fetch_add(1);
    std::string pname = makeProcessName(idx);
    auto instrs = generateRandomInstructions(pname,
                    static_cast<int>(cfg_.minIns),
                    static_cast<int>(cfg_.maxIns));
    auto proc = std::make_shared<Process>(pname, idx, std::move(instrs));
    allProcesses_.push_back(proc);
    readyQueue_.push(proc);
}

// ── FCFS dispatch ─────────────────────────────────────────────────────────────
void Scheduler::dispatchFCFS() {
    for (auto& core : cores_) {
        if (core.busy || readyQueue_.empty()) continue;
        auto proc = readyQueue_.front();
        readyQueue_.pop();
        proc->state.store(ProcState::RUNNING);
        proc->coreId.store(core.id);
        core.assigned = proc;
        core.busy = true;
    }
}

// ── RR dispatch ───────────────────────────────────────────────────────────────
void Scheduler::dispatchRR() {
    for (int i = 0; i < cfg_.numCpu; ++i) {
        auto& core = cores_[i];
        if (!core.busy && !readyQueue_.empty()) {
            auto proc = readyQueue_.front();
            readyQueue_.pop();
            proc->state.store(ProcState::RUNNING);
            proc->coreId.store(core.id);
            core.assigned = proc;
            core.busy     = true;
            quantumLeft_[i] = cfg_.quantumCycles;
        }
    }
}

// ── Main scheduling loop ──────────────────────────────────────────────────────
void Scheduler::mainLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (shutdownRequested_) return;

            uint64_t tick = cpuCycles_.load();

            // 1. Batch generation
            if (batchRunning_ && cfg_.batchProcessFreq > 0 &&
                tick % cfg_.batchProcessFreq == 0) {
                generateBatchProcess();
            }

            // 2. Execute one instruction per busy core
            for (int i = 0; i < cfg_.numCpu; ++i) {
                auto& core = cores_[i];
                if (!core.busy || !core.assigned) continue;
                auto& proc = core.assigned;

                // Handle SLEEP
                if (proc->state.load() == ProcState::SLEEPING) {
                    if (proc->canResume(tick)) {
                        proc->state.store(ProcState::RUNNING);
                    } else {
                        // Preempt sleeping process — put back in ready queue
                        // so other processes can run.
                        readyQueue_.push(proc);
                        core.assigned = nullptr;
                        core.busy     = false;
                        continue;
                    }
                }

                // delay-per-exec: skip execution for `delayPerExec` ticks
                // (simple busy-wait: process holds the core but does nothing)
                // Track per-core delay separately if needed; for simplicity,
                // treat delayPerExec as 0 here and always execute.
                // (Full implementation: add a per-core delay counter.)

                if (coreDelayCounter_[i] > 0) {
                    // Busy-wait: process holds the core but executes nothing this tick.
                    --coreDelayCounter_[i];
                } else {
                    bool alive = proc->executeNextInstruction(i, tick);

                    if (!alive || proc->state.load() == ProcState::FINISHED) {
                        proc->state.store(ProcState::FINISHED);
                        finished_.push_back(proc);
                        core.assigned = nullptr;
                        core.busy     = false;
                        coreDelayCounter_[i] = 0;               // reset on finish
                    } else if (cfg_.scheduler == "rr") {
                        if (--quantumLeft_[i] == 0) {
                            proc->state.store(ProcState::READY);
                            proc->coreId.store(-1);
                            readyQueue_.push(proc);
                            core.assigned   = nullptr;
                            core.busy       = false;
                            quantumLeft_[i] = cfg_.quantumCycles;
                            coreDelayCounter_[i] = 0;           // reset on preempt
                        } else {
                            coreDelayCounter_[i] = cfg_.delayPerExec; // arm delay for next instr
                        }
                    } else {
                        // FCFS
                        coreDelayCounter_[i] = cfg_.delayPerExec;     // arm delay for next instr
                    }
                }
            }

            // 3. Dispatch free cores
            if (cfg_.scheduler == "fcfs") dispatchFCFS();
            else                           dispatchRR();

            cpuCycles_.fetch_add(1);
        }
        // Small sleep to avoid pegging the CPU 100% on the host machine.
        // Remove / reduce if benchmarking scheduler throughput.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}