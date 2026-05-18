#pragma once

#include "physical_register_file.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <set>
#include <vector>

namespace ooo {

struct RenameResult {
    bool writes = false;
    int architecturalRegister = -1;
    int newPhysicalRegister = -1;
    int oldPhysicalRegister = -1;
};

// Register Alias Table plus free-list. x0 is permanently mapped to p0.
class RenameTable {
public:
    static constexpr int kArchitecturalRegisters = 32;

    explicit RenameTable(int physicalRegisterCount = 64);

    void reset();

    int currentMapping(int architecturalRegister) const;
    int committedMapping(int architecturalRegister) const;

    std::array<int, kArchitecturalRegisters> snapshot() const;
    void restore(const std::array<int, kArchitecturalRegisters>& snapshot);

    bool canAllocate() const;
    std::size_t freeCount() const;
    int physicalRegisterCount() const;
    std::vector<int> freeListSnapshot() const;
    RenameResult allocateDestination(int architecturalRegister, PhysicalRegisterFile& registerFile);

    void commit(int architecturalRegister, int physicalRegister);
    void rebuildFreeList(const std::set<int>& usedPhysicalRegisters);

private:
    int physicalRegisterCount_;
    std::array<int, kArchitecturalRegisters> currentMap_{};
    std::array<int, kArchitecturalRegisters> committedMap_{};
    std::deque<int> freeList_;
};

} // namespace ooo
