CSOPESY Major Output 1 - Process Multiplexer and CLI

Team members:
- Jeff Cumti
- Rain Dulatre
- Joanna Hong
- Robbie Pineda

Entry point:
src/main.cpp

Requirements:
- C++17 compiler
- CMake 3.16 or newer, or GNU Make with g++

Build with CMake on Windows:
1. Open PowerShell in the project root.
2. Run: cmake -S . -B build
3. Run: cmake --build build --config Release
4. Run: .\build\Release\csopesy.exe

Build with CMake on Linux/macOS:
1. Open a terminal in the project root.
2. Run: cmake -S . -B build
3. Run: cmake --build build
4. Run: ./build/csopesy

Build with GNU Make:
1. Open a terminal in the project root.
2. Run: make
3. Run ./csopesy_scheduler on Linux/macOS, or
   .\csopesy_scheduler.exe on Windows.

The executable must be run with config.txt available in the current project
directory. Type "initialize" before using scheduler or screen commands.

Main commands:
- initialize
- screen -s <process name>
- screen -r <process name>
- screen -ls
- scheduler-start
- scheduler-stop
- report-util
- exit
