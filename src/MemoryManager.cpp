// src/MemoryManager.cpp
#include "MemoryManager.h"
#include <algorithm>

MemoryManager::MemoryManager(uint32_t totalMem, uint32_t memPerProc)
    : totalMem_(totalMem), memPerProc_(memPerProc) {
    blocks_.push_back({0, totalMem_, true, ""});
}

bool MemoryManager::allocate(const std::string& procName, uint32_t& outStart) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
        auto& b = blocks_[i];
        if (!b.free || b.size < memPerProc_) continue;

        outStart = b.start;
        if (b.size == memPerProc_) {
            b.free = false;
            b.procName = procName;
        } else {
            MemoryBlock allocated{b.start, memPerProc_, false, procName};
            b.start += memPerProc_;
            b.size  -= memPerProc_;
            blocks_.insert(blocks_.begin() + static_cast<long>(i), allocated);
        }
        return true;
    }
    return false;
}

void MemoryManager::free(const std::string& procName) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& b : blocks_) {
        if (!b.free && b.procName == procName) {
            b.free = true;
            b.procName.clear();
            break;
        }
    }
    mergeFreeBlocks();
}

void MemoryManager::mergeFreeBlocks() {
    std::sort(blocks_.begin(), blocks_.end(),
              [](const MemoryBlock& a, const MemoryBlock& b) {
                  return a.start < b.start;
              });
    for (std::size_t i = 0; i + 1 < blocks_.size();) {
        if (blocks_[i].free && blocks_[i + 1].free) {
            blocks_[i].size += blocks_[i + 1].size;
            blocks_.erase(blocks_.begin() + static_cast<long>(i) + 1);
        } else {
            ++i;
        }
    }
}

uint32_t MemoryManager::totalExternalFragmentation() const {
    std::lock_guard<std::mutex> lk(mutex_);
    uint32_t total = 0;
    for (auto& b : blocks_) if (b.free) total += b.size;
    return total;
}

int MemoryManager::numProcessesInMemory() const {
    std::lock_guard<std::mutex> lk(mutex_);
    int n = 0;
    for (auto& b : blocks_) if (!b.free) ++n;
    return n;
}

std::vector<MemoryBlock> MemoryManager::allocatedBlocksDescending() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<MemoryBlock> result;
    for (auto& b : blocks_) if (!b.free) result.push_back(b);
    std::sort(result.begin(), result.end(),
              [](const MemoryBlock& a, const MemoryBlock& b) {
                  return a.start > b.start;
              });
    return result;
}