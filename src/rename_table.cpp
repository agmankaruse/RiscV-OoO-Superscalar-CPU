#include "rename_table.hpp"

#include <stdexcept>

namespace ooo {

RenameTable::RenameTable(int physicalRegisterCount)
    : physicalRegisterCount_(physicalRegisterCount) {
    reset();
}

void RenameTable::reset() {
    freeList_.clear();
    for (int i = 0; i < kArchitecturalRegisters; ++i) {
        currentMap_[i] = i;
        committedMap_[i] = i;
    }
    for (int i = kArchitecturalRegisters; i < physicalRegisterCount_; ++i) {
        freeList_.push_back(i);
    }
}

int RenameTable::currentMapping(int architecturalRegister) const {
    return currentMap_.at(static_cast<std::size_t>(architecturalRegister));
}

int RenameTable::committedMapping(int architecturalRegister) const {
    return committedMap_.at(static_cast<std::size_t>(architecturalRegister));
}

std::array<int, RenameTable::kArchitecturalRegisters> RenameTable::snapshot() const {
    return currentMap_;
}

void RenameTable::restore(const std::array<int, kArchitecturalRegisters>& snapshot) {
    currentMap_ = snapshot;
    currentMap_[0] = 0;
}

bool RenameTable::canAllocate() const {
    return !freeList_.empty();
}

std::size_t RenameTable::freeCount() const {
    return freeList_.size();
}

int RenameTable::physicalRegisterCount() const {
    return physicalRegisterCount_;
}

std::vector<int> RenameTable::freeListSnapshot() const {
    return {freeList_.begin(), freeList_.end()};
}

RenameResult RenameTable::allocateDestination(int architecturalRegister, PhysicalRegisterFile& registerFile) {
    RenameResult result;
    if (architecturalRegister <= 0) {
        return result;
    }
    if (!canAllocate()) {
        throw std::runtime_error("physical register free-list exhausted");
    }

    result.writes = true;
    result.architecturalRegister = architecturalRegister;
    result.oldPhysicalRegister = currentMap_[architecturalRegister];
    result.newPhysicalRegister = freeList_.front();
    freeList_.pop_front();

    currentMap_[architecturalRegister] = result.newPhysicalRegister;
    registerFile.setReady(result.newPhysicalRegister, false);
    return result;
}

void RenameTable::commit(int architecturalRegister, int physicalRegister) {
    if (architecturalRegister <= 0) {
        return;
    }
    committedMap_[architecturalRegister] = physicalRegister;
}

void RenameTable::rebuildFreeList(const std::set<int>& usedPhysicalRegisters) {
    freeList_.clear();
    for (int phys = 1; phys < physicalRegisterCount_; ++phys) {
        if (usedPhysicalRegisters.count(phys) == 0) {
            freeList_.push_back(phys);
        }
    }
}

} // namespace ooo
