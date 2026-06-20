// utils.h - small free-function helpers shared across the program.
#pragma once

#include <string>

namespace utils {

// Thread-safe wrapper around std::localtime(). Returns a timestamp
// formatted like "08/06/2024 09:15:22AM".
std::string getCurrentTimestamp();

// Strips leading/trailing whitespace (spaces, tabs, \r, \n).
std::string trim(const std::string& s);

} // namespace utils
