// src/Scheduler.cpp
#include "Scheduler.h"
#include "Instruction.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

Scheduler::Scheduler(const Config& cfg) : cfg_(cfg) {
    cores_.resize(cfg_.numCpu);
    quantumLeft_.resize(cfg_.numCpu, cfg_.quantumCycles);
    for (int i = 0; i < cfg_.numCpu; ++i) cores_[i].id = i;
}

Scheduler::~Scheduler() { shutdown(); }

std::string Scheduler::makeProcessName(int idx) {
    std::ostringstream oss;
    oss << "p" << std::setfill('0') << std::setw(2) << idx;
    return oss.str();
}

int Scheduler::allocateProcessId() {
    return nextProcessId_.fetch_add(1);
}

void Scheduler::addProcess(std::shared_ptr<Process> proc) {
    std::lock_guard<std::mutex> lk(mutex_);
    const bool duplicate = std::any_of(
        allProcesses_.begin(),
        allProcesses_.end(),
        [&proc](const auto& existing) {
            return existing->name == proc->name;
        });
    if (duplicate) {
        throw std::invalid_argument(
            "process name '" + proc->name + "' already exists");
    }
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
    for (const auto& proc : allProcesses_) {
        if (proc->state.load() != ProcState::RUNNING) continue;
        const int coreId = proc->coreId.load();
        if (coreId < 0) continue;
        snap.runningProcesses.push_back({
            proc->name,
            proc->id,
            coreId,
            proc->currentInstruction.load(),
            proc->totalInstructions,
            proc->getStartTimestamp()
        });
    }
    for (const auto& proc : finished_) {
        snap.finishedProcesses.push_back({
            proc->name,
            proc->id,
            -1,
            proc->totalInstructions,
            proc->totalInstructions,
            proc->getStartTimestamp()
        });
    }
    snap.allProcessesInOrder = allProcesses_;
    snap.finishedInOrder     = finished_;
    return snap;
}

void Scheduler::startBatchGeneration() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (batchRunning_.load()) return;
    batchRunning_.store(true);
    nextBatchGenerationTick_ =
        cpuCycles_.load() + static_cast<uint64_t>(cfg_.batchProcessFreq);
}

void Scheduler::stopBatchGeneration() {
    std::lock_guard<std::mutex> lk(mutex_);
    batchRunning_.store(false);
}

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
    std::string pname;
    bool nameExists = false;
    do {
        pname = makeProcessName(nextBatchNameIndex_++);
        nameExists = std::any_of(
            allProcesses_.begin(),
            allProcesses_.end(),
            [&pname](const auto& process) {
                return process->name == pname;
            });
    } while (nameExists);

    auto instrs = generateRandomInstructions(pname,
                    cfg_.minIns,
                    cfg_.maxIns);
    auto proc = std::make_shared<Process>(
        pname, allocateProcessId(), std::move(instrs));
    allProcesses_.push_back(proc);
    readyQueue_.push(proc);
}

void Scheduler::wakeSleepingProcesses(uint64_t tick) {
    auto sleeping = sleepingProcesses_.begin();
    while (sleeping != sleepingProcesses_.end()) {
        auto& proc = *sleeping;
        if (proc->canResume(tick)) {
            proc->state.store(ProcState::READY);
            readyQueue_.push(proc);
            sleeping = sleepingProcesses_.erase(sleeping);
        } else {
            ++sleeping;
        }
    }
}

void Scheduler::releaseCore(int coreIndex) {
    auto& core = cores_[coreIndex];
    core.assigned = nullptr;
    core.busy = false;
    quantumLeft_[coreIndex] = cfg_.quantumCycles;
}

void Scheduler::dispatchFreeCores() {
    if (cfg_.scheduler == "fcfs") dispatchFCFS();
    else dispatchRR();
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

            // 1. Wake blocked processes whose SLEEP interval has elapsed.
            wakeSleepingProcesses(tick);

            // 2. Batch generation is relative to scheduler-start.
            if (batchRunning_.load() && tick >= nextBatchGenerationTick_) {
                try {
                    generateBatchProcess();
                } catch (const std::exception& error) {
                    batchRunning_.store(false);
                    std::cerr << "Error: batch process generation stopped: "
                              << error.what() << '\n';
                }
                nextBatchGenerationTick_ =
                    tick + static_cast<uint64_t>(cfg_.batchProcessFreq);
            }

            // 3. Dispatch free cores.
            dispatchFreeCores();

            // 4. Execute one occupied CPU tick per busy core.
            for (int i = 0; i < cfg_.numCpu; ++i) {
                auto& core = cores_[i];
                if (!core.busy || !core.assigned) continue;
                auto& proc = core.assigned;

                bool executedInstruction = false;
                if (!proc->consumeDelayTick()) {
                    executedInstruction = true;
                    bool alive = proc->executeNextInstruction(i, tick);

                    if (!alive || proc->state.load() == ProcState::FINISHED) {
                        proc->state.store(ProcState::FINISHED);
                        proc->coreId.store(-1);
                        proc->clearDelay();
                        finished_.push_back(proc);
                        releaseCore(i);
                        continue;
                    }

                    if (proc->state.load() == ProcState::SLEEPING) {
                        proc->coreId.store(-1);
                        sleepingProcesses_.push_back(proc);
                        releaseCore(i);
                        continue;
                    }

                    proc->armDelay(cfg_.delayPerExec);
                }

                // A round-robin time slice is measured in occupied CPU ticks,
                // including delay-per-exec busy-wait ticks.
                if (cfg_.scheduler == "rr") {
                    if (quantumLeft_[i] > 0) --quantumLeft_[i];
                    if (quantumLeft_[i] == 0) {
                        proc->state.store(ProcState::READY);
                        proc->coreId.store(-1);
                        readyQueue_.push(proc);
                        releaseCore(i);
                    } else if (!executedInstruction) {
                        proc->state.store(ProcState::RUNNING);
                    }
                } else {
                    proc->state.store(ProcState::RUNNING);
                }
            }

            // 5. Make cores released by completion, SLEEP, or RR preemption
            // immediately available in the completed-tick snapshot.
            dispatchFreeCores();

            cpuCycles_.fetch_add(1);
        }
        // CPU ticks are simulated. This sleep only prevents the emulator from
        // monopolizing the host CPU and does not change simulated utilization.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
