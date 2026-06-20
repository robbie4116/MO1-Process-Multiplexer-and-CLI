# CSOPESY Week 6 Homework — FCFS Scheduler + PRINT-to-file

Implements the specific homework requirements (the screenshots), built as a
standalone piece of the larger OS Emulator described in the project spec PDF.

## File layout

| File                            | Responsibility                                                              |
|----------------------------------|------------------------------------------------------------------------------|
| `config.h`                     | Tunable constants (cores, process/instruction counts, output dir).          |
| `utils.h` / `utils.cpp`        | `getCurrentTimestamp()`, `trim()` — small free-function helpers.            |
| `process.h` / `process.cpp`    | `Process`: holds its own state/progress and knows how to run itself (the PRINT-instruction loop + writing its log file). |
| `scheduler.h` / `scheduler.cpp`| `Scheduler`: the FCFS ready queue, the scheduler thread, and the per-core worker threads. Exposes a thread-safe `getSnapshot()` for the UI. |
| `console_ui.h` / `console_ui.cpp` | `printBanner()` and `printScreenLs()` — all terminal output/formatting.   |
| `main.cpp`                       | Wires it together: creates the test-case workload, starts the scheduler, runs the command loop. |

## Build & run

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)     # builds all .cpp files and links csopesy_scheduler
./csopesy_scheduler
```

No setup/`initialize` step is required for this homework — on launch the
program immediately creates the test-case workload and starts scheduling it.

## What happens on startup

1. **10 processes** (`p01`–`p10`) are created, each with **100 PRINT
   instructions**, and placed in FCFS order in the ready queue.
2. The scheduler (4 cores, FCFS, non-preemptive) starts dispatching them
   immediately — **no further user input is needed** for processes to run
   to completion.
3. Each process writes its own log file to `process_logs/<name>.txt` as it
   executes, in the required format:
   ```
   Process name: p01
   Logs:

   (06/20/2026 09:15:22PM) Core:0 "Hello world from p01!"
   ...
   ```

## Commands

| Command       | Behavior                                                          |
|---------------|--------------------------------------------------------------------|
| `screen -ls`  | Shows CPU utilization, cores used/available, and the running/finished process lists (format matches the reference mockup in the spec). |
| `exit`        | Stops the console and joins all threads.                          |

## Recording the required test case

1. Launch `./csopesy_scheduler` — the 10 processes (100 prints each) are
   created automatically.
2. Type `screen -ls` every 1–2 seconds until all 10 processes appear under
   **Finished processes**.
3. Type `exit`.
4. Zip the `process_logs/` folder for submission:
   ```bash
   cd process_logs && zip -r ../process_logs.zip . && cd ..
   ```

## Threading model

- **1 scheduler thread** (`Scheduler::schedulerLoop`) — owns the FCFS ready
  queue, and whenever a core is free, hands it the next queued process (in
  arrival order).
- **1 worker thread per core** (`Scheduler::coreWorkerLoop`, 4 total) — each
  waits for an assignment, calls `Process::run()` to execute that process to
  completion (FCFS is non-preemptive, so a process keeps its core until its
  100 instructions are done), then reports back and waits for the next job.
- All shared scheduling state (ready queue, core busy flags, finished list)
  lives inside `Scheduler` behind a single mutex + condition variable, so
  the scheduler and workers never race on it. `getSnapshot()` takes that
  lock once and hands the UI layer a consistent, read-only copy. Each
  `Process` also has its own small mutex for its start timestamp, plus
  atomics for its live progress counters, so `screen -ls` can safely read
  in-flight progress from the main thread while a worker is writing to it.

## Notes / things to adjust before reusing this for the full project

- `config.h` hardcodes 4 cores / 10 processes / 100 prints each, as the
  homework calls for, rather than reading from `config.txt` (`config.txt`,
  `initialize`, `scheduler-start/stop`, `report-util`, and `screen -r` are
  part of the bigger project and intentionally left out of this homework
  scope).
- Per the assignment's "IMPORTANT NOTE," the file-writing in the PRINT
  instruction (`Process::run()`) is meant for grading this homework only —
  disable/guard it before wiring this into your full machine-project
  submission, since per-instruction file I/O will affect scheduler
  performance at scale.
