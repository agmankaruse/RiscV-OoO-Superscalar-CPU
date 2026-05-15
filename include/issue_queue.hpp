#pragma once

#include "instruction.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace ooo {

struct IssueEntry {
    std::uint64_t robId = 0;
    Instruction instruction;
    int src1Physical = -1;
    int src2Physical = -1;
    int destPhysical = -1;
};

class IssueQueue {
public:
    explicit IssueQueue(std::size_t capacity = 16);

    bool canAllocate() const;
    void add(const IssueEntry& entry);
    void remove(std::uint64_t robId);
    void flushYoungerThan(std::uint64_t robId);
    void clear();

    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;

    const std::vector<IssueEntry>& entries() const;

private:
    std::size_t capacity_;
    std::vector<IssueEntry> entries_;
};

} // namespace ooo
