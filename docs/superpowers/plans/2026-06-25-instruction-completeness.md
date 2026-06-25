# Instruction Completeness Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement variable-aware `PRINT`, nested `FOR`, randomized loops, wide counters, and accurate submission documentation.

**Architecture:** Expand nested instruction trees into a flat executable stream at process construction, keeping scheduler execution unchanged. Generate loop trees within an exact expanded-instruction budget and expose a seeded generator overload for deterministic tests.

**Tech Stack:** C++17, CMake/CTest, standard library containers and random generators.

---

### Task 1: Add failing instruction semantics tests

**Files:**
- Modify: `tests/SchedulerTests.cpp`

- [ ] Test `PRINT` with a declared variable.
- [ ] Test `PRINT` auto-declares an unknown variable at zero.
- [ ] Test nested `FOR` expansion and execution.
- [ ] Test nesting deeper than three levels is rejected.
- [ ] Test seeded generation includes `FOR` while preserving exact expanded count.
- [ ] Run tests and verify RED.

### Task 2: Implement instruction expansion and variable printing

**Files:**
- Modify: `include/Instruction.h`
- Modify: `include/SimProcess.h`
- Modify: `src/Process.cpp`

- [ ] Add optional print-variable metadata.
- [ ] Recursively expand `FOR` trees with a depth limit of three.
- [ ] Use 64-bit instruction progress counters.
- [ ] Evaluate the optional variable while formatting PRINT logs.
- [ ] Run tests and verify GREEN.

### Task 3: Generate randomized FOR programs

**Files:**
- Modify: `include/Instruction.h`
- Modify: `src/Instruction.cpp`
- Modify: `src/Scheduler.cpp`
- Modify: `src/ConsoleManager.cpp`

- [ ] Accept `uint32_t` instruction bounds without signed narrowing.
- [ ] Add deterministic seeded generation.
- [ ] Generate loops within an exact expanded-count budget.
- [ ] Catch process generation failures at CLI/batch boundaries.
- [ ] Run all tests.

### Task 4: Complete submission documentation

**Files:**
- Create: `README.txt`
- Modify: `README.md`

- [ ] Add team names, entry file, CMake and Make build/run instructions.
- [ ] Correct scheduler and instruction architecture descriptions.
- [ ] Remove stale claims and examples.

### Task 5: Final verification and direct push

- [ ] Run Debug and Release CTest.
- [ ] Run MinGW Make build and clean.
- [ ] Run exact four-core scenario.
- [ ] Run `git diff --check`.
- [ ] Pull/rebase `origin/FINAL_MO1`.
- [ ] Commit and push directly to `FINAL_MO1`.
