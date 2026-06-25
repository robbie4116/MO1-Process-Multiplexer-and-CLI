// include/ConfigParser.h
#pragma once
#include "Config.h"
#include <string>

class ConfigParser {
public:
    // Returns true on success. Populates 'out'.
    static bool parse(const std::string& filename, Config& out);
};