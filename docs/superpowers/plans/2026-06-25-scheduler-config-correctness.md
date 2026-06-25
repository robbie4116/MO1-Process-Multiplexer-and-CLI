# Scheduler and Configuration Correctness Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make scheduler behavior faithfully follow `config.txt`, keep all available simulated CPUs occupied when runnable work exists, and reject invalid black-box test configurations safely.

**Architecture:** Keep the existing single scheduler-loop design, but separate runnable and sleeping process state. Each tick wakes eligible sleepers, generates scheduled batch work, dispatches runnable work, executes occupied cores, releases/preempts processes, and redispatches newly freed cores before publishing the completed tick state. Add a small C++ test executable using deterministic instructions and scheduler snapshots.

**Tech Stack:** C++17, CMake/CTest, standard library threads, mutexes, atomics, queues, and assertions.

---

## Chunk 1: Scheduler regression coverage

### Task 1: Add deterministic scheduler tests

**Files:**
- Create: `tests/SchedulerTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `include/Scheduler.h`

- [ ] Add a scheduler test target with CTest.
- [ ] Add deterministic process construction helpers for non-sleeping and sleeping instruction sequences.
- [ ] Add a failing test proving all configured cores remain assigned when runnable backlog exceeds `num-cpu`.
- [ ] Add a failing test proving a sleeping process relinquishes its core and is not dispatched again before its wake tick.
- [ ] Add a failing test proving RR quantum expires after occupied CPU ticks when `delay-per-exec` is nonzero.
- [ ] Run `cmake --build build --config Debug` and `ctest --test-dir build -C Debug --output-on-failure`; verify failures reflect current scheduler behavior.

## Chunk 2: Correct scheduler state transitions

### Task 2: Separate sleeping processes from the ready queue

**Files:**
- Modify: `include/Scheduler.h`
- Modify: `src/Scheduler.cpp`
- Modify: `src/Process.cpp`

- [ ] Add a sleeping-process collection and wake eligible processes at the start of each tick.
- [ ] When `SLEEP` executes, clear the process core ID, move it to sleeping state, and release the core immediately.
- [ ] Ensure dispatch only receives `READY` processes.
- [ ] Redispatch after execution so any released core is assigned before the completed-tick snapshot can be read.
- [ ] Count RR quantum on every occupied tick, including busy-wait delay ticks.
- [ ] Run scheduler tests and verify they pass.

### Task 3: Make batch timing relative to `scheduler-start`

**Files:**
- Modify: `include/Scheduler.h`
- Modify: `src/Scheduler.cpp`
- Test: `tests/SchedulerTests.cpp`

- [ ] Add a failing test that batch frequency `N` does not generate immediately and generates after `N` completed ticks.
- [ ] Record the next generation tick when batch generation starts.
- [ ] Generate once per configured interval and reset timing when restarted.
- [ ] Run scheduler tests and verify they pass.

## Chunk 3: Configuration and observable CLI correctness

### Task 4: Validate all configuration parameters

**Files:**
- Modify: `src/ConfigParser.cpp`
- Test: `tests/ConfigParserTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] Add failing tests for missing keys, malformed values, `num-cpu` outside `[1,128]`, unsupported scheduler values, zero-only-disallowed parameters, and `min-ins > max-ins`.
- [ ] Parse into a temporary config and reject invalid input without partially initializing the emulator.
- [ ] Accept the specification spelling `delay-per-exec`; optionally accept `delays-per-exec` as a compatibility alias.
- [ ] Run config parser tests and verify they pass.

### Task 5: Correct process identifiers and status output

**Files:**
- Modify: `include/Scheduler.h`
- Modify: `src/Scheduler.cpp`
- Modify: `src/ConsoleManager.cpp`
- Test: `tests/SchedulerTests.cpp`

- [ ] Add a failing test proving manual and generated processes receive unique IDs.
- [ ] Allocate IDs inside `Scheduler::addProcess` or through a single scheduler-owned allocator.
- [ ] List only core-assigned processes under running processes.
- [ ] Return the specification-required `Process <name> not found.` response for both absent and finished processes.
- [ ] Run tests and verify they pass.

## Chunk 4: Black-box verification and audit closeout

### Task 6: Verify varied configurations without recompilation

**Files:**
- Create: `tests/black_box_scheduler.ps1`
- Modify: `CMakeLists.txt`

- [ ] Build the executable once.
- [ ] Run parameter cases for 1, 2, 4, and 8 CPUs under RR and FCFS.
- [ ] Verify `screen -ls` reports the configured core count and reaches full utilization whenever runnable backlog is sufficient.
- [ ] Verify batch frequency, quantum, and delay settings affect behavior without recompilation.
- [ ] Verify `scheduler-stop` stops new generation while existing work continues.
- [ ] Verify `report-util` matches `screen -ls` fields and includes running and finished processes.
- [ ] Run `ctest --test-dir build -C Debug --output-on-failure`, the PowerShell black-box script, `cmake --build build --config Release`, and `git diff --check`.

### Task 7: Report remaining specification gaps

**Files:**
- No production changes.

- [ ] Re-audit all six PDF pages against the final branch.
- [ ] Report still-unimplemented items separately, especially nested `FOR`, variable-aware `PRINT`, submission `README.txt`, and Makefile portability if not included in this scope.
- [ ] Inspect for leftover emulator and Node-based test processes and conservatively clean obvious orphans.
