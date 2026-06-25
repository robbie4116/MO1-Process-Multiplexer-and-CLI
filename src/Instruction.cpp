// src/Instruction.cpp
#include "Instruction.h"
#include <random>
#include <ctime>

static std::mt19937& rng() {
    static std::mt19937 engine(static_cast<unsigned>(std::time(nullptr)));
    return engine;
}

static int randInt(int lo, int hi) {          // inclusive both ends
    return std::uniform_int_distribution<int>(lo, hi)(rng());
}

// Build a PRINT instruction for "Hello world from <name>!"
static std::shared_ptr<Instruction> makePrint(const std::string& name) {
    auto instr = std::make_shared<Instruction>();
    instr->type     = InstrType::PRINT;
    instr->printMsg = "Hello world from " + name + "!";
    return instr;
}

static std::shared_ptr<Instruction> makeDeclare(const std::string& var, uint16_t val) {
    auto instr = std::make_shared<Instruction>();
    instr->type      = InstrType::DECLARE;
    instr->declVar   = var;
    instr->declValue = val;
    return instr;
}

static std::shared_ptr<Instruction> makeSleep(uint8_t ticks) {
    auto instr = std::make_shared<Instruction>();
    instr->type       = InstrType::SLEEP;
    instr->sleepTicks = ticks;
    return instr;
}

std::vector<std::shared_ptr<Instruction>>
generateRandomInstructions(const std::string& processName,
                           int minCount, int maxCount) {
    int count = randInt(minCount, maxCount);
    std::vector<std::shared_ptr<Instruction>> instrs;
    instrs.reserve(count);

    // Always start with a DECLARE so ADD/SUBTRACT have a variable to work with.
    instrs.push_back(makeDeclare("x", 0));

    for (int i = 1; i < count; ++i) {
        int choice = randInt(0, 3);   // 0=PRINT, 1=ADD, 2=SUBTRACT, 3=SLEEP
        switch (choice) {
        case 0:
            instrs.push_back(makePrint(processName));
            break;
        case 1: {
            auto instr = std::make_shared<Instruction>();
            instr->type      = InstrType::ADD;
            instr->arithVar1 = "x";
            instr->arithVar2 = "x"; instr->arithIsLit2 = false;
            instr->arithLit3 = static_cast<uint16_t>(randInt(1, 10));
            instr->arithIsLit3 = true;
            instrs.push_back(instr);
            break;
        }
        case 2: {
            auto instr = std::make_shared<Instruction>();
            instr->type      = InstrType::SUBTRACT;
            instr->arithVar1 = "x";
            instr->arithVar2 = "x"; instr->arithIsLit2 = false;
            instr->arithLit3 = static_cast<uint16_t>(randInt(1, 5));
            instr->arithIsLit3 = true;
            instrs.push_back(instr);
            break;
        }
        case 3:
            instrs.push_back(makeSleep(static_cast<uint8_t>(randInt(1, 5))));
            break;
        }
    }
    return instrs;
}
