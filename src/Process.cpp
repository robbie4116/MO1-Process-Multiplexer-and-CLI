// src/Process.cpp
#include "SimProcess.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

uint64_t expandedInstructionCount(
    const std::vector<std::shared_ptr<Instruction>>& instructions,
    int loopDepth) {
    uint64_t total = 0;
    for (const auto& instruction : instructions) {
        if (!instruction) {
            throw std::invalid_argument("instruction cannot be null");
        }

        uint64_t contribution = 1;
        if (instruction->type == InstrType::FOR) {
            if (loopDepth >= 3) {
                throw std::invalid_argument(
                    "FOR instructions may be nested at most three levels");
            }
            if (instruction->forBody.empty() || instruction->forRepeats == 0) {
                throw std::invalid_argument(
                    "FOR requires a non-empty body and at least one repeat");
            }
            const uint64_t bodyCount =
                expandedInstructionCount(instruction->forBody, loopDepth + 1);
            if (bodyCount >
                std::numeric_limits<uint64_t>::max() / instruction->forRepeats) {
                throw std::length_error("expanded FOR instruction count overflow");
            }
            contribution = bodyCount * instruction->forRepeats;
        }

        if (total > std::numeric_limits<uint64_t>::max() - contribution) {
            throw std::length_error("expanded instruction count overflow");
        }
        total += contribution;
    }
    return total;
}

void appendExpandedInstructions(
    const std::vector<std::shared_ptr<Instruction>>& instructions,
    int loopDepth,
    std::vector<std::shared_ptr<Instruction>>& output) {
    for (const auto& instruction : instructions) {
        if (instruction->type != InstrType::FOR) {
            output.push_back(instruction);
            continue;
        }

        for (uint32_t repeat = 0; repeat < instruction->forRepeats; ++repeat) {
            appendExpandedInstructions(
                instruction->forBody, loopDepth + 1, output);
        }
    }
}

} // namespace

Process::Process(std::string name_, int id_,
                 std::vector<std::shared_ptr<Instruction>> instructions)
    : name(std::move(name_)),
      id(id_),
      instructions_(expandInstructions(instructions)) {
    variables_["x"] = 0;
    variables_["y"] = 0;
    variables_["z"] = 0;
    totalInstructions = static_cast<uint64_t>(instructions_.size());
}

std::vector<std::shared_ptr<Instruction>> Process::expandInstructions(
    const std::vector<std::shared_ptr<Instruction>>& instructions) {
    const uint64_t count = expandedInstructionCount(instructions, 0);
    if (count > std::vector<std::shared_ptr<Instruction>>().max_size()) {
        throw std::length_error(
            "expanded instructions exceed addressable container capacity");
    }

    std::vector<std::shared_ptr<Instruction>> expanded;
    expanded.reserve(static_cast<std::size_t>(count));
    appendExpandedInstructions(instructions, 0, expanded);
    return expanded;
}

std::string Process::getStartTimestamp() const {
    std::lock_guard<std::mutex> lk(metaMutex_);
    return startTimestamp_;
}

void Process::setStartTimestamp(const std::string& ts) {
    std::lock_guard<std::mutex> lk(metaMutex_);
    if (startTimestamp_.empty()) startTimestamp_ = ts;
}

std::vector<std::string> Process::getLogs() const {
    std::lock_guard<std::mutex> lk(metaMutex_);
    return logs_;
}

void Process::appendLog(const std::string& entry) {
    std::lock_guard<std::mutex> lk(metaMutex_);
    logs_.push_back(entry);
}

uint16_t& Process::getOrDeclareVar(const std::string& varName) {
    auto it = variables_.find(varName);
    if (it == variables_.end()) {
        variables_[varName] = 0;   // auto-declare at 0
    }
    return variables_[varName];
}

bool Process::canResume(uint64_t currentTick) const {
    return currentTick >= sleepUntilTick_.load();
}

bool Process::consumeDelayTick() {
    uint32_t remaining = delayTicksRemaining_.load();
    while (remaining > 0) {
        if (delayTicksRemaining_.compare_exchange_weak(
                remaining, remaining - 1)) {
            return true;
        }
    }
    return false;
}

void Process::armDelay(uint32_t ticks) {
    delayTicksRemaining_.store(ticks);
}

void Process::clearDelay() {
    delayTicksRemaining_.store(0);
}

bool Process::executeNextInstruction(int coreId_, uint64_t currentTick) {
    uint64_t idx = currentInstruction.load();
    if (idx >= totalInstructions) {
        variables_.clear();
        state.store(ProcState::FINISHED);
        return false;
    }

    setStartTimestamp(utils::getCurrentTimestamp());
    state.store(ProcState::RUNNING);
    this->coreId.store(coreId_);

    auto& instr = instructions_[static_cast<std::size_t>(idx)];
    std::string ts = utils::getCurrentTimestamp();

    switch (instr->type) {
    case InstrType::PRINT: {
        std::ostringstream oss;
        oss << "(" << ts << ") Core:" << coreId_ << " \"" << instr->printMsg;
        if (instr->printHasVar) {
            oss << getOrDeclareVar(instr->printVar);
        }
        oss << "\"";
        appendLog(oss.str());
        break;
    }
    case InstrType::DECLARE:
        variables_[instr->declVar] = instr->declValue;
        break;
    case InstrType::ADD: {
        uint16_t op2 = instr->arithIsLit2 ? instr->arithLit2 : getOrDeclareVar(instr->arithVar2);
        uint16_t op3 = instr->arithIsLit3 ? instr->arithLit3 : getOrDeclareVar(instr->arithVar3);
        uint32_t result = static_cast<uint32_t>(op2) + static_cast<uint32_t>(op3);
        getOrDeclareVar(instr->arithVar1) =
            static_cast<uint16_t>(std::min(result, static_cast<uint32_t>(UINT16_MAX)));
        break;
    }
    case InstrType::SUBTRACT: {
        uint16_t op2 = instr->arithIsLit2 ? instr->arithLit2 : getOrDeclareVar(instr->arithVar2);
        uint16_t op3 = instr->arithIsLit3 ? instr->arithLit3 : getOrDeclareVar(instr->arithVar3);
        getOrDeclareVar(instr->arithVar1) =
            static_cast<uint16_t>(op2 > op3 ? op2 - op3 : 0);   // clamp at 0
        break;
    }
    case InstrType::SLEEP:
        sleepUntilTick_.store(currentTick + instr->sleepTicks);
        state.store(ProcState::SLEEPING);
        currentInstruction.fetch_add(1);
        return true;   // still alive, just sleeping
    case InstrType::FOR:
        throw std::logic_error("FOR must be expanded before execution");
    }

    uint64_t next = currentInstruction.fetch_add(1) + 1;
    if (next >= totalInstructions) {
        variables_.clear();
        state.store(ProcState::FINISHED);
        return false;
    }
    return true;
}
