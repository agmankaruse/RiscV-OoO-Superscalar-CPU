#include "functional_unit.hpp"

#include <stdexcept>

namespace ooo {

FunctionalUnit::FunctionalUnit(FunctionalUnitType type, int latencyCycles)
    : type_(type), latencyCycles_(latencyCycles) {}

FunctionalUnitType FunctionalUnit::type() const {
    return type_;
}

bool FunctionalUnit::busy() const {
    return operation_.has_value();
}

int FunctionalUnit::remainingCycles() const {
    return remainingCycles_;
}

void FunctionalUnit::issue(const IssuedOperation& operation) {
    if (busy()) {
        throw std::runtime_error("functional unit already busy");
    }
    operation_ = operation;
    remainingCycles_ = latencyCycles_;
}

std::optional<IssuedOperation> FunctionalUnit::tick() {
    if (!operation_) {
        return std::nullopt;
    }
    --remainingCycles_;
    if (remainingCycles_ > 0) {
        return std::nullopt;
    }

    auto completed = operation_;
    operation_.reset();
    remainingCycles_ = 0;
    return completed;
}

void FunctionalUnit::clear() {
    operation_.reset();
    remainingCycles_ = 0;
}

FunctionalUnitType unitTypeFor(const Instruction& instruction) {
    if (instruction.isMemory()) {
        return FunctionalUnitType::LoadStore;
    }
    if (instruction.isControl()) {
        return FunctionalUnitType::Branch;
    }
    return FunctionalUnitType::Integer;
}

} // namespace ooo
