// include/MemoryManager.h
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

struct MemoryBlock {
    uint32_t    start;
    uint32_t    size;
    bool        free;
    std::string procName;
};

// Flat, contiguous, first-fit memory allocator (no paging, no backing store).
class MemoryManager {
public:
    MemoryManager(uint32_t totalMem, uint32_t memPerProc);

    // First-fit allocate a mem-per-proc-sized block for procName.
    // Returns true and sets outStart on success.
    bool allocate(const std::string& procName, uint32_t& outStart);

    // Frees the block owned by procName (no-op if not found).
    void free(const std::string& procName);

    uint32_t totalExternalFragmentation() const;
    int      numProcessesInMemory() const;
    uint32_t totalMemory() const { return totalMem_; }

    // Allocated blocks only, sorted top-of-memory to bottom (descending start).
    std::vector<MemoryBlock> allocatedBlocksDescending() const;

private:
    void mergeFreeBlocks();

    uint32_t totalMem_;
    uint32_t memPerProc_;
    std::vector<MemoryBlock> blocks_;
    mutable std::mutex mutex_;
};