cmake_minimum_required(VERSION 3.16)
project(CSOpesyEmulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CSOPESY_CORE_SOURCES
    src/ConfigParser.cpp
    src/ConsoleManager.cpp
    src/Instruction.cpp
    src/MemoryManager.cpp
    src/Process.cpp
    src/ProcessTable.cpp
    src/Scheduler.cpp
    src/Utils.cpp
)

add_library(csopesy_core ${CSOPESY_CORE_SOURCES})
target_include_directories(csopesy_core PUBLIC include)
target_compile_features(csopesy_core PUBLIC cxx_std_17)

add_executable(csopesy src/main.cpp)
target_link_libraries(csopesy PRIVATE csopesy_core)

find_package(Threads REQUIRED)
target_link_libraries(csopesy_core PUBLIC Threads::Threads)

include(CTest)
if(BUILD_TESTING)
    add_executable(csopesy_tests tests/SchedulerTests.cpp)
    target_link_libraries(csopesy_tests PRIVATE csopesy_core)
    add_test(NAME csopesy_tests COMMAND csopesy_tests)
    if(WIN32)
        add_test(
            NAME csopesy_black_box
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File ${CMAKE_CURRENT_SOURCE_DIR}/tests/black_box_scheduler.ps1
                -Executable $<TARGET_FILE:csopesy>
        )
    endif()
endif()