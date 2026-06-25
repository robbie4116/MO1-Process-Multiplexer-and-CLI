// src/Process.cpp
#include "SimProcess.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <sstream>

Process::Process(std::string name_, int id_,
                 std::vector<std::shared_ptr<Instruction>> instructions)
    : name(std::move(name_)),
      id(id_),
      totalInstructions(static_cast<int>(instructions.size())),
      instructions_(std::move(instructions)) {}

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

bool Process::executeNextInstruction(int coreId_, uint64_t currentTick) {
    int idx = currentInstruction.load();
    if (idx >= totalInstructions) {
        state.store(ProcState::FINISHED);
        return false;
    }

    setStartTimestamp(utils::getCurrentTimestamp());
    state.store(ProcState::RUNNING);
    this->coreId.store(coreId_);

    auto& instr = instructions_[idx];
    std::string ts = utils::getCurrentTimestamp();

    switch (instr->type) {
    case InstrType::PRINT: {
        std::ostringstream oss;
        oss << "(" << ts << ") Core:" << coreId_ << " \"" << instr->printMsg << "\"";
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
        // FOR is unrolled at generation time for this implementation.
        break;
    }

    int next = currentInstruction.fetch_add(1) + 1;
    if (next >= totalInstructions) {
        state.store(ProcState::FINISHED);
        return false;
    }
    return true;
}
