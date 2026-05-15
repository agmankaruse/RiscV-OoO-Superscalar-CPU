#include "physical_register_file.hpp"

#include <algorithm>
#include <stdexcept>

namespace ooo {

PhysicalRegisterFile::PhysicalRegisterFile(int registerCount)
    : values_(registerCount, 0), ready_(registerCount, true) {}

void PhysicalRegisterFile::reset() {
    std::fill(values_.begin(), values_.end(), 0);
    std::fill(ready_.begin(), ready_.end(), true);
}

std::uint32_t PhysicalRegisterFile::read(int physicalRegister) const {
    if (physicalRegister < 0 || physicalRegister >= size()) {
        throw std::out_of_range("physical register read out of range");
    }
    if (physicalRegister == 0) {
        return 0;
    }
    return values_[physicalRegister];
}

void PhysicalRegisterFile::write(int physicalRegister, std::uint32_t value) {
    if (physicalRegister < 0 || physicalRegister >= size()) {
        throw std::out_of_range("physical register write out of range");
    }
    if (physicalRegister == 0) {
        values_[0] = 0;
        ready_[0] = true;
        return;
    }
    values_[physicalRegister] = value;
    ready_[physicalRegister] = true;
}

bool PhysicalRegisterFile::isReady(int physicalRegister) const {
    if (physicalRegister < 0 || physicalRegister >= size()) {
        return true;
    }
    if (physicalRegister == 0) {
        return true;
    }
    return ready_[physicalRegister];
}

void PhysicalRegisterFile::setReady(int physicalRegister, bool ready) {
    if (physicalRegister < 0 || physicalRegister >= size()) {
        throw std::out_of_range("physical register readiness out of range");
    }
    ready_[physicalRegister] = physicalRegister == 0 ? true : ready;
}

int PhysicalRegisterFile::size() const {
    return static_cast<int>(values_.size());
}

} // namespace ooo
