#include "Utils.h"

#include <chrono>
#include <ctime>
#include <mutex>

namespace {
// std::localtime() writes into a shared internal buffer and is not
// thread-safe, so every call is funneled through this mutex.
std::mutex g_timeMutex;
} // namespace

namespace utils {

std::string getCurrentTimestamp() {
    using namespace std::chrono;
    std::time_t t = system_clock::to_time_t(system_clock::now());
    std::tm tmCopy{};
    {
        std::lock_guard<std::mutex> lock(g_timeMutex);
        std::tm* tmp = std::localtime(&t);
        if (tmp) tmCopy = *tmp;
    }
    char buf[32];
    // e.g. 08/06/2024 09:15:22AM
    std::strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", &tmCopy);
    return std::string(buf);
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace utils
