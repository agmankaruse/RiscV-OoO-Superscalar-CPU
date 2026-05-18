#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <vector>

namespace ooo {

struct LoadStoreEntry {
    std::uint64_t robId = 0;
    bool isLoad = false;
    bool addressReady = false;
    bool valueReady = false;
    std::uint32_t address = 0;
    std::uint32_t value = 0;
};

struct LoadIssueInfo {
    bool canIssue = false;
    bool hasForwardedValue = false;
    std::uint32_t forwardedValue = 0;
};

// The LSQ is intentionally simple: stores update memory at commit, while loads
// may issue early only when all older store addresses are known.
class LoadStoreQueue {
public:
    explicit LoadStoreQueue(std::size_t capacity = 16);

    bool canAllocate() const;
    void addLoad(std::uint64_t robId);
    void addStore(std::uint64_t robId);

    void markLoadComplete(std::uint64_t robId, std::uint32_t address, std::uint32_t value);
    void markStoreReady(std::uint64_t robId, std::uint32_t address, std::uint32_t value);

    LoadIssueInfo canLoadIssue(std::uint64_t robId, std::uint32_t address) const;

    void retire(std::uint64_t robId);
    void flushYoungerThan(std::uint64_t robId);
    void clear();

    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;
    const std::vector<LoadStoreEntry>& entries() const;

private:
    LoadStoreEntry* find(std::uint64_t robId);
    const LoadStoreEntry* find(std::uint64_t robId) const;

    std::size_t capacity_;
    std::vector<LoadStoreEntry> entries_;
};

} // namespace ooo
