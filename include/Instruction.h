// include/Instruction.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

enum class InstrType { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

struct Instruction {
    InstrType type;

    // PRINT
    std::string printMsg;   // literal part, e.g. "Hello world from p01!"
    bool        printHasVar = false;
    std::string printVar;

    // DECLARE
    std::string declVar;
    uint16_t    declValue = 0;

    // ADD / SUBTRACT  =>  var1 = operand2 OP operand3
    std::string arithVar1;
    std::string arithVar2;   // variable name OR empty if literal
    uint16_t    arithLit2 = 0;
    bool        arithIsLit2 = false;
    std::string arithVar3;
    uint16_t    arithLit3 = 0;
    bool        arithIsLit3 = false;

    // SLEEP
    uint8_t sleepTicks = 0;

    // FOR
    std::vector<std::shared_ptr<Instruction>> forBody;
    uint32_t forRepeats = 1;
};

// Generates an instruction tree whose expanded executable instruction count
// is within the inclusive configured range.
std::vector<std::shared_ptr<Instruction>>
generateRandomInstructions(const std::string& processName,
                           uint32_t minCount, uint32_t maxCount);

// Deterministic overload used by tests and reproducible demonstrations.
std::vector<std::shared_ptr<Instruction>>
generateRandomInstructions(const std::string& processName,
                           uint32_t minCount, uint32_t maxCount,
                           uint32_t seed);
