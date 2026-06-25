// include/Config.h
#pragma once
#include <string>
#include <cstdint>

struct Config {
    int      numCpu          = 4;
    std::string scheduler    = "rr";
    uint32_t quantumCycles   = 5;
    uint32_t batchProcessFreq= 1;
    uint32_t minIns          = 1000;
    uint32_t maxIns          = 2000;
    uint32_t delayPerExec    = 0;
};