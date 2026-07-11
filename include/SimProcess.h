// include/SimProcess.h
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include "Instruction.h"

enum class ProcState { READY, RUNNING, SLEEPING, FINISHED };

class Process {
public:
    Process(std::string name, int id,
            std::vector<std::shared_ptr<Instruction>> instructions);

    // Execute ONE instruction on the given core.
    // Returns false if the process is finished.
    // Sets sleepUntilTick_ if a SLEEP is encountered.
    bool executeNextInstruction(int coreId, uint64_t currentTick);

    // True when SLEEP period has expired.
    bool canResume(uint64_t currentTick) const;

    // delay-per-exec busy-wait state follows the process across preemption.
    bool consumeDelayTick();
    void armDelay(uint32_t ticks);
    void clearDelay();

    // Thread-safe log access (for process-smi display).
    std::vector<std::string> getLogs() const;
    std::string getStartTimestamp() const;

    const std::string name;
    const int id;
    uint64_t totalInstructions = 0;

    std::atomic<uint64_t>  currentInstruction{0};
    std::atomic<int>       coreId{-1};
    std::atomic<ProcState> state{ProcState::READY};
    std::atomic<bool>      inMemory{false};

private:
    uint16_t& getOrDeclareVar(const std::string& varName);
    void appendLog(const std::string& entry);
    void setStartTimestamp(const std::string& ts);
    static std::vector<std::shared_ptr<Instruction>> expandInstructions(
        const std::vector<std::shared_ptr<Instruction>>& instructions);

    std::vector<std::shared_ptr<Instruction>> instructions_;
    std::unordered_map<std::string, uint16_t>  variables_;
    std::vector<std::string>                    logs_;       // in-memory PRINT output
    std::atomic<uint64_t>                       sleepUntilTick_{0};
    std::atomic<uint32_t>                       delayTicksRemaining_{0};

    mutable std::mutex metaMutex_;
    std::string        startTimestamp_;
};
