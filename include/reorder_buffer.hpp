#pragma once

#include "instruction.hpp"
#include "rename_table.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

namespace ooo {

struct RobEntry {
    std::uint64_t id = 0;
    Instruction instruction;

    bool ready = false;
    std::uint32_t value = 0;

    bool writesRegister = false;
    int architecturalDestination = -1;
    int physicalDestination = -1;
    int oldPhysicalDestination = -1;

    std::uint32_t storeAddress = 0;
    std::uint32_t storeValue = 0;

    int predictedNextPc = -1;
    bool hasRenameSnapshot = false;
    std::array<int, RenameTable::kArchitecturalRegisters> renameSnapshot{};
};

class ReorderBuffer {
public:
    explicit ReorderBuffer(std::size_t capacity = 32);

    void clear();
    bool canAllocate() const;
    std::size_t capacity() const;
    std::size_t size() const;
    bool empty() const;

    std::uint64_t allocate(RobEntry entry);
    RobEntry* find(std::uint64_t id);
    const RobEntry* find(std::uint64_t id) const;

    RobEntry& head();
    const RobEntry& head() const;
    void popHead();

    std::vector<RobEntry> flushYoungerThan(std::uint64_t id);

    const std::deque<RobEntry>& entries() const;

private:
    std::size_t capacity_;
    std::deque<RobEntry> entries_;
};

} // namespace ooo
