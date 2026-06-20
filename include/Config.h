// config.h - tunable constants for the homework test case.
#pragma once

#include <string>

namespace config {

constexpr int NUM_CORES = 4;                  // Requirement #4: declare 4 cores
constexpr int NUM_PROCESSES = 10;             // Test case: 10 processes
constexpr int INSTRUCTIONS_PER_PROCESS = 100; // Test case: 100 print commands each
constexpr int INSTRUCTION_DELAY_MS = 15;      // Simulated per-instruction CPU time

inline const std::string OUTPUT_DIR = "process_logs";

} // namespace config
