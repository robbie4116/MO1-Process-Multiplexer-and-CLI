#include "Process.h"

#include <chrono>
#include <fstream>
#include <thread>
#include <iostream>

#include "Config.h"
#include "Utils.h"

Process::Process(std::string name_, int id_, int totalInstructions_)
    : name(std::move(name_)), id(id_), totalInstructions(totalInstructions_) {}

std::string Process::getStartTimestamp() const {
    std::lock_guard<std::mutex> lk(metaMutex_);
    return startTimestamp_;
}

void Process::setStartTimestamp(const std::string& ts) {
    std::lock_guard<std::mutex> lk(metaMutex_);
    startTimestamp_ = ts;
}

void Process::run(int coreId) {
    setStartTimestamp(utils::getCurrentTimestamp());
    state = ProcState::RUNNING;

    std::ofstream outFile(config::OUTPUT_DIR + "/" + name + ".txt");
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open log file for " << name << "\n";
        state = ProcState::FINISHED;
        return;
    }
    outFile << "Process name: " << name << "\n";
    outFile << "Logs:\n\n";

    for (int i = 0; i < totalInstructions; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config::INSTRUCTION_DELAY_MS));
        std::string ts = utils::getCurrentTimestamp();
        outFile << "(" << ts << ") Core:" << coreId
                << " \"Hello world from " << name << "!\"\n";
        outFile.flush();
        currentInstruction.store(i + 1);
    }
    outFile.close();
    state = ProcState::FINISHED;
}
