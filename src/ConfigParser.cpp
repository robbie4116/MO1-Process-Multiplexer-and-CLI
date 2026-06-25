// src/ConfigParser.cpp
#include "ConfigParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

bool ConfigParser::parse(const std::string& filename, Config& out) {
    std::vector<std::string> candidates = {
        filename,
        "../" + filename
    };

    std::ifstream file;
    std::string usedPath;
    for (auto& path : candidates) {
        file.open(path);
        if (file.is_open()) { usedPath = path; break; }
    }

    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << filename
                  << " (searched current dir and ../)\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;

        if (key == "num-cpu")           { iss >> out.numCpu; }
        else if (key == "scheduler")    { iss >> out.scheduler;
            // strip surrounding quotes if present
            if (!out.scheduler.empty() && out.scheduler.front() == '"')
                out.scheduler = out.scheduler.substr(1, out.scheduler.size()-2); }
        else if (key == "quantum-cycles")      { iss >> out.quantumCycles; }
        else if (key == "batch-process-freq")  { iss >> out.batchProcessFreq; }
        else if (key == "min-ins")             { iss >> out.minIns; }
        else if (key == "max-ins")             { iss >> out.maxIns; }
        else if (key == "delay-per-exec")      { iss >> out.delayPerExec; }
    }
    return true;
}
