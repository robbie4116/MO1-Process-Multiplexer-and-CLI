// src/ConfigParser.cpp
#include "ConfigParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
#include <set>

namespace {

bool parseUnsigned(const std::string& text, uint32_t& value) {
    try {
        std::size_t consumed = 0;
        const auto parsed = std::stoull(text, &consumed);
        if (consumed != text.size() ||
            parsed > std::numeric_limits<uint32_t>::max()) {
            return false;
        }
        value = static_cast<uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool fail(const std::string& message) {
    std::cerr << "Error: invalid config.txt: " << message << '\n';
    return false;
}

} // namespace

bool ConfigParser::parse(const std::string& filename, Config& out) {
    std::ifstream file;
    for (const auto& path : {filename, "../" + filename}) {
        file.clear();
        file.open(path);
        if (file.is_open()) break;
    }

    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << filename
                  << " (searched current dir and ../)\n";
        return false;
    }

    Config parsed;
    std::set<std::string> found;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key, value, extra;
        if (!(iss >> key >> value) || (iss >> extra)) {
            return fail("malformed line " + std::to_string(lineNumber));
        }

        if (key == "delays-per-exec") key = "delay-per-exec";
        if (!found.insert(key).second) {
            return fail("duplicate key '" + key + "'");
        }

        if (key == "num-cpu") {
            uint32_t number = 0;
            if (!parseUnsigned(value, number) || number < 1 || number > 128) {
                return fail("num-cpu must be in [1, 128]");
            }
            parsed.numCpu = static_cast<int>(number);
        } else if (key == "scheduler") {
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            if (value != "fcfs" && value != "rr") {
                return fail("scheduler must be \"fcfs\" or \"rr\"");
            }
            parsed.scheduler = value;
        } else if (key == "quantum-cycles") {
            if (!parseUnsigned(value, parsed.quantumCycles) ||
                parsed.quantumCycles == 0) {
                return fail("quantum-cycles must be at least 1");
            }
        } else if (key == "batch-process-freq") {
            if (!parseUnsigned(value, parsed.batchProcessFreq) ||
                parsed.batchProcessFreq == 0) {
                return fail("batch-process-freq must be at least 1");
            }
        } else if (key == "min-ins") {
            if (!parseUnsigned(value, parsed.minIns) || parsed.minIns == 0) {
                return fail("min-ins must be at least 1");
            }
        } else if (key == "max-ins") {
            if (!parseUnsigned(value, parsed.maxIns) || parsed.maxIns == 0) {
                return fail("max-ins must be at least 1");
            }
        } else if (key == "delay-per-exec") {
            if (!parseUnsigned(value, parsed.delayPerExec)) {
                return fail("delay-per-exec must be a uint32 value");
            }
        } else if (key == "delay-per-exec") {
            if (!parseUnsigned(value, parsed.delayPerExec)) {
                return fail("delay-per-exec must be a uint32 value");
            }
        } else if (key == "max-overall-mem") {
            if (!parseUnsigned(value, parsed.maxOverallMem) ||
                parsed.maxOverallMem == 0) {
                return fail("max-overall-mem must be at least 1");
            }
        } else if (key == "mem-per-frame") {
            if (!parseUnsigned(value, parsed.memPerFrame) ||
                parsed.memPerFrame == 0) {
                return fail("mem-per-frame must be at least 1");
            }
        } else if (key == "mem-per-proc") {
            if (!parseUnsigned(value, parsed.memPerProc) ||
                parsed.memPerProc == 0) {
                return fail("mem-per-proc must be at least 1");
            }
        } else {
            return fail("unknown key '" + key + "'");
        }
    }

    static const std::set<std::string> required{
        "num-cpu", "scheduler", "quantum-cycles", "batch-process-freq",
        "min-ins", "max-ins", "delay-per-exec",
        "max-overall-mem", "mem-per-frame", "mem-per-proc"
    };
    if (found != required) return fail("one or more required keys are missing");
    if (parsed.minIns > parsed.maxIns) {
        return fail("min-ins must not exceed max-ins");
    }
    if (parsed.minIns > parsed.maxIns) {
        return fail("min-ins must not exceed max-ins");
    }
    if (parsed.memPerProc > parsed.maxOverallMem) {
        return fail("mem-per-proc must not exceed max-overall-mem");
    }
    
    out = parsed;
    return true;
}
