// src/Instruction.cpp
#include "Instruction.h"
#include <algorithm>
#include <iterator>
#include <mutex>
#include <random>
#include <stdexcept>
#include <ctime>

static std::mt19937& rng() {
    static std::mt19937 engine(static_cast<unsigned>(std::time(nullptr)));
    return engine;
}

static std::mutex& rngMutex() {
    static std::mutex mutex;
    return mutex;
}

static std::shared_ptr<Instruction> makePrint(
    const std::string& name,
    std::mt19937&) {
    auto instr = std::make_shared<Instruction>();
    instr->type     = InstrType::PRINT;
    instr->printMsg = "Hello world from " + name + "!";
    return instr;
}

static std::shared_ptr<Instruction> makeSleep(uint8_t ticks) {
    auto instr = std::make_shared<Instruction>();
    instr->type       = InstrType::SLEEP;
    instr->sleepTicks = ticks;
    return instr;
}

static std::shared_ptr<Instruction> makeSimpleInstruction(
    const std::string& processName,
    std::mt19937& engine) {
    const int choice = std::uniform_int_distribution<int>(0, 3)(engine);
    if (choice == 0) return makePrint(processName, engine);
    if (choice == 3) {
        return makeSleep(static_cast<uint8_t>(
            std::uniform_int_distribution<int>(1, 5)(engine)));
    }

    auto instr = std::make_shared<Instruction>();
    instr->type = choice == 1 ? InstrType::ADD : InstrType::SUBTRACT;
    instr->arithVar1 = "x";
    instr->arithVar2 = "x";
    instr->arithIsLit2 = false;
    instr->arithLit3 = static_cast<uint16_t>(
        choice == 1
            ? std::uniform_int_distribution<int>(1, 10)(engine)
            : std::uniform_int_distribution<int>(1, 5)(engine));
    instr->arithIsLit3 = true;
    return instr;
}

static std::vector<std::shared_ptr<Instruction>> generateBlock(
    const std::string& processName,
    uint32_t expandedBudget,
    int loopDepth,
    std::mt19937& engine) {
    std::vector<std::shared_ptr<Instruction>> instructions;
    uint32_t remaining = expandedBudget;

    while (remaining > 0) {
        const bool canGenerateFor = loopDepth < 3 && remaining >= 2;
        const bool chooseFor = canGenerateFor &&
            std::uniform_int_distribution<int>(0, 4)(engine) == 0;

        if (!chooseFor) {
            instructions.push_back(makeSimpleInstruction(processName, engine));
            --remaining;
            continue;
        }

        const uint32_t maxRepeats = std::min<uint32_t>(4, remaining);
        const uint32_t repeats =
            std::uniform_int_distribution<uint32_t>(2, maxRepeats)(engine);
        const uint32_t maxBodyBudget =
            std::min<uint32_t>(6, remaining / repeats);
        const uint32_t bodyBudget =
            std::uniform_int_distribution<uint32_t>(1, maxBodyBudget)(engine);

        auto loop = std::make_shared<Instruction>();
        loop->type = InstrType::FOR;
        loop->forRepeats = repeats;
        loop->forBody =
            generateBlock(processName, bodyBudget, loopDepth + 1, engine);
        instructions.push_back(std::move(loop));
        remaining -= bodyBudget * repeats;
    }
    return instructions;
}

static std::vector<std::shared_ptr<Instruction>> generateWithEngine(
    const std::string& processName,
    uint32_t minCount,
    uint32_t maxCount,
    std::mt19937& engine) {
    if (minCount == 0 || minCount > maxCount) {
        throw std::invalid_argument("invalid instruction count range");
    }

    const uint32_t count =
        std::uniform_int_distribution<uint32_t>(minCount, maxCount)(engine);
    return generateBlock(processName, count, 0, engine);
}

std::vector<std::shared_ptr<Instruction>>
generateRandomInstructions(const std::string& processName,
                           uint32_t minCount, uint32_t maxCount) {
    std::lock_guard<std::mutex> lock(rngMutex());
    return generateWithEngine(processName, minCount, maxCount, rng());
}

std::vector<std::shared_ptr<Instruction>>
generateRandomInstructions(const std::string& processName,
                           uint32_t minCount, uint32_t maxCount,
                           uint32_t seed) {
    std::mt19937 engine(seed);
    return generateWithEngine(processName, minCount, maxCount, engine);
}
