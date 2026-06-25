# Windows Build Portability Fix Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore a successful Windows/MSVC build from the current `FINAL_MO1` branch.

**Architecture:** Keep application behavior unchanged. Add the standard header required by `std::vector`, and rename the project `Process.h` header so Windows' case-insensitive include lookup cannot confuse it with the C runtime `<process.h>` header.

**Tech Stack:** C++17, CMake, MSVC/MSBuild

---

## Chunk 1: Build portability repair

### Task 1: Repair standard-library header resolution

**Files:**
- Modify: `src/ConfigParser.cpp`
- Rename: `include/Process.h` to `include/SimProcess.h`
- Modify: `src/Process.cpp`
- Modify: `include/Scheduler.h`
- Modify: `README.md`

- [x] **Step 1: Run the failing regression command**

Run: `cmake --build .` from `build/`

Expected before the fix: FAIL with `std::vector` missing and project `process.h` shadowing the Windows runtime header.

- [x] **Step 2: Apply the minimal implementation**

Add `#include <vector>` to `ConfigParser.cpp`, rename the conflicting project header, and update all references.

- [x] **Step 3: Verify from a clean CMake configuration**

Run a fresh configure and `cmake --build . --config Debug`.

Expected after the fix: PASS and produce `csopesy.exe`.

- [x] **Step 4: Inspect and commit**

Run `git diff --check`, review the staged diff, commit, and push to `origin/FINAL_MO1`.
