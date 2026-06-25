#include "ConfigParser.h"
#include "Instruction.h"
#include "Scheduler.h"
#include "SimProcess.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::shared_ptr<Instruction> printInstruction(const std::string& message = "test") {
    auto instruction = std::make_shared<Instruction>();
    instruction->type = InstrType::PRINT;
    instruction->printMsg = message;
    return instruction;
}

std::shared_ptr<Instruction> sleepInstruction(uint8_t ticks) {
    auto instruction = std::make_shared<Instruction>();
    instruction->type = InstrType::SLEEP;
    instruction->sleepTicks = ticks;
    return instruction;
}

std::shared_ptr<Process> processWithInstructions(
    const std::string& name,
    int id,
    const std::vector<std::shared_ptr<Instruction>>& instructions) {
    return std::make_shared<Process>(name, id, instructions);
}

std::shared_ptr<Process> longRunningProcess(const std::string& name, int id) {
    std::vector<std::shared_ptr<Instruction>> instructions(10000);
    for (auto& instruction : instructions) instruction = printInstruction();
    return processWithInstructions(name, id, instructions);
}

bool waitUntil(const std::function<bool()>& predicate,
               std::chrono::milliseconds timeout = 2000ms) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(1ms);
    }
    return predicate();
}

void waitForTick(Scheduler& scheduler, uint64_t tick) {
    require(waitUntil([&] { return scheduler.getSnapshot().cpuCycles >= tick; }),
            "scheduler did not reach expected tick");
}

Config baseConfig() {
    Config config;
    config.numCpu = 4;
    config.scheduler = "rr";
    config.quantumCycles = 5;
    config.batchProcessFreq = 1;
    config.minIns = 1000;
    config.maxIns = 2000;
    config.delayPerExec = 0;
    return config;
}

void testAllCoresStayOccupiedWithRunnableBacklog() {
    auto config = baseConfig();
    Scheduler scheduler(config);
    for (int i = 0; i < 12; ++i) {
        scheduler.addProcess(longRunningProcess("work" + std::to_string(i), i + 1));
    }
    scheduler.start();
    waitForTick(scheduler, 5);

    for (int sample = 0; sample < 50; ++sample) {
        const auto snapshot = scheduler.getSnapshot();
        require(snapshot.coresUsed == config.numCpu,
                "runnable backlog left a configured core idle");
        std::this_thread::sleep_for(1ms);
    }
    scheduler.shutdown();
}

void testSnapshotRunningRowsMatchAssignedCores() {
    auto config = baseConfig();
    Scheduler scheduler(config);
    for (int i = 0; i < 12; ++i) {
        scheduler.addProcess(longRunningProcess("work" + std::to_string(i), i + 1));
    }
    scheduler.start();
    waitForTick(scheduler, 5);

    for (int sample = 0; sample < 100; ++sample) {
        const auto snapshot = scheduler.getSnapshot();
        require(static_cast<int>(snapshot.runningProcesses.size()) ==
                    snapshot.coresUsed,
                "snapshot running rows did not match assigned cores");
        for (const auto& process : snapshot.runningProcesses) {
            require(process.coreId >= 0 && process.coreId < config.numCpu,
                    "snapshot contained an invalid running core ID");
        }
        std::this_thread::sleep_for(1ms);
    }
    scheduler.shutdown();
}

void testSleepingProcessRelinquishesCoreUntilWakeTick() {
    auto config = baseConfig();
    config.numCpu = 1;
    config.quantumCycles = 20;

    std::vector<std::shared_ptr<Instruction>> instructions{
        sleepInstruction(40),
        printInstruction("awake"),
    };
    auto sleeper = processWithInstructions("sleeper", 1, instructions);
    Scheduler scheduler(config);
    scheduler.addProcess(sleeper);
    scheduler.addProcess(longRunningProcess("worker", 2));
    scheduler.start();

    require(waitUntil([&] { return sleeper->state.load() == ProcState::SLEEPING; }),
            "process never entered sleeping state");
    const auto sleepingAt = scheduler.getSnapshot().cpuCycles;
    require(sleeper->coreId.load() == -1,
            "sleeping process retained a CPU core assignment");

    waitForTick(scheduler, sleepingAt + 10);
    require(sleeper->state.load() == ProcState::SLEEPING,
            "sleeping process became runnable before its wake tick");
    require(sleeper->coreId.load() == -1,
            "sleeping process was dispatched before its wake tick");
    scheduler.shutdown();
}

void testRoundRobinQuantumCountsBusyWaitTicks() {
    auto config = baseConfig();
    config.numCpu = 1;
    config.quantumCycles = 2;
    config.delayPerExec = 3;

    auto first = longRunningProcess("first", 1);
    auto second = longRunningProcess("second", 2);
    Scheduler scheduler(config);
    scheduler.addProcess(first);
    scheduler.addProcess(second);
    scheduler.start();

    waitForTick(scheduler, 4);
    require(!second->getStartTimestamp().empty(),
            "RR quantum ignored occupied busy-wait CPU ticks");
    scheduler.shutdown();
}

void testBusyWaitDelaySurvivesRoundRobinPreemption() {
    auto config = baseConfig();
    config.numCpu = 1;
    config.quantumCycles = 2;
    config.delayPerExec = 3;

    auto first = longRunningProcess("first", 1);
    auto second = longRunningProcess("second", 2);
    Scheduler scheduler(config);
    scheduler.addProcess(first);
    scheduler.addProcess(second);
    scheduler.start();

    waitForTick(scheduler, 6);
    require(first->currentInstruction.load() == 1,
            "RR preemption discarded the process's remaining busy-wait delay");
    scheduler.shutdown();
}

void testBatchGenerationWaitsForConfiguredInterval() {
    auto config = baseConfig();
    config.numCpu = 1;
    config.batchProcessFreq = 8;
    Scheduler scheduler(config);
    scheduler.start();
    const auto startedAt = scheduler.getSnapshot().cpuCycles;
    scheduler.startBatchGeneration();

    waitForTick(scheduler, startedAt + config.batchProcessFreq - 1);
    require(scheduler.getSnapshot().allProcessesInOrder.empty(),
            "batch generation occurred before the configured interval elapsed");

    waitForTick(scheduler, startedAt + config.batchProcessFreq + 1);
    require(!scheduler.getSnapshot().allProcessesInOrder.empty(),
            "batch generation did not occur after the configured interval");
    scheduler.shutdown();
}

void testSchedulerStopHaltsBatchGeneration() {
    auto config = baseConfig();
    config.numCpu = 1;
    config.batchProcessFreq = 1;
    config.minIns = 1;
    config.maxIns = 1;
    Scheduler scheduler(config);
    scheduler.start();
    scheduler.startBatchGeneration();

    waitForTick(scheduler, 10);
    scheduler.stopBatchGeneration();
    const auto countAtStop =
        scheduler.getSnapshot().allProcessesInOrder.size();
    waitForTick(scheduler, 20);
    require(scheduler.getSnapshot().allProcessesInOrder.size() == countAtStop,
            "scheduler-stop did not halt batch generation");
    scheduler.shutdown();
}

void testManualAndGeneratedProcessesUseUniqueIds() {
    auto config = baseConfig();
    config.numCpu = 1;
    config.batchProcessFreq = 1;
    config.minIns = 100;
    config.maxIns = 100;
    Scheduler scheduler(config);
    const int manualId = scheduler.allocateProcessId();
    scheduler.addProcess(longRunningProcess("manual", manualId));
    scheduler.start();
    scheduler.startBatchGeneration();

    require(waitUntil([&] {
        return scheduler.getSnapshot().allProcessesInOrder.size() >= 2;
    }), "generated process was not created");
    const auto snapshot = scheduler.getSnapshot();
    require(snapshot.allProcessesInOrder[0]->id !=
                snapshot.allProcessesInOrder[1]->id,
            "manual and generated processes reused the same ID");
    scheduler.shutdown();
}

void writeConfig(const std::filesystem::path& path, const std::string& body) {
    std::ofstream file(path);
    file << body;
}

std::string validConfig() {
    return
        "num-cpu 4\n"
        "scheduler \"rr\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 1\n"
        "min-ins 1000\n"
        "max-ins 2000\n"
        "delay-per-exec 0\n";
}

void testConfigValidation() {
    const auto directory =
        std::filesystem::temp_directory_path() / "csopesy-config-tests";
    std::filesystem::create_directories(directory);
    const auto path = directory / "config.txt";

    Config parsed;
    writeConfig(path, validConfig());
    require(ConfigParser::parse(path.string(), parsed), "valid config was rejected");
    require(parsed.numCpu == 4 && parsed.scheduler == "rr",
            "valid config values were not parsed");

    writeConfig(path,
        "num-cpu 0\n"
        "scheduler \"rr\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 1\n"
        "min-ins 1000\n"
        "max-ins 2000\n"
        "delay-per-exec 0\n");
    require(!ConfigParser::parse(path.string(), parsed),
            "out-of-range num-cpu was accepted");

    writeConfig(path,
        "num-cpu 4\n"
        "scheduler \"invalid\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 1\n"
        "min-ins 1000\n"
        "max-ins 2000\n"
        "delay-per-exec 0\n");
    require(!ConfigParser::parse(path.string(), parsed),
            "unsupported scheduler was accepted");

    writeConfig(path,
        "num-cpu 4\n"
        "scheduler \"fcfs\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 1\n"
        "min-ins 2000\n"
        "max-ins 1000\n"
        "delay-per-exec 0\n");
    require(!ConfigParser::parse(path.string(), parsed),
            "min-ins greater than max-ins was accepted");

    writeConfig(path,
        "num-cpu 4\n"
        "scheduler \"rr\"\n"
        "quantum-cycles 5\n");
    require(!ConfigParser::parse(path.string(), parsed),
            "config with missing required keys was accepted");

    std::filesystem::remove_all(directory);
}

void run(const char* name, const std::function<void()>& test, int& failures) {
    try {
        test();
        std::cout << "[PASS] " << name << '\n';
    } catch (const std::exception& error) {
        ++failures;
        std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
    }
}

} // namespace

int main() {
    int failures = 0;
    run("all cores stay occupied with runnable backlog",
        testAllCoresStayOccupiedWithRunnableBacklog, failures);
    run("snapshot running rows match assigned cores",
        testSnapshotRunningRowsMatchAssignedCores, failures);
    run("sleeping process relinquishes core until wake tick",
        testSleepingProcessRelinquishesCoreUntilWakeTick, failures);
    run("RR quantum counts busy-wait ticks",
        testRoundRobinQuantumCountsBusyWaitTicks, failures);
    run("busy-wait delay survives RR preemption",
        testBusyWaitDelaySurvivesRoundRobinPreemption, failures);
    run("batch generation waits for configured interval",
        testBatchGenerationWaitsForConfiguredInterval, failures);
    run("scheduler-stop halts batch generation",
        testSchedulerStopHaltsBatchGeneration, failures);
    run("manual and generated processes use unique IDs",
        testManualAndGeneratedProcessesUseUniqueIds, failures);
    run("configuration values are validated",
        testConfigValidation, failures);
    return failures == 0 ? 0 : 1;
}
