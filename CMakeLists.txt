cmake_minimum_required(VERSION 3.16)
project(CSOpesyEmulator)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(csopesy ${SOURCES})

target_include_directories(csopesy PRIVATE include)

find_package(Threads REQUIRED)
target_link_libraries(csopesy Threads::Threads)
