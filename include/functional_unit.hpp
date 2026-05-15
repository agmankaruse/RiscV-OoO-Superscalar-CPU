#pragma once

#include "instruction.hpp"

#include <cstdint>
#include <optional>

namespace ooo {

enum class FunctionalUnitType {
    Integer,
    Branch,
    LoadStore
};

struct IssuedOperation {
    std::uint64_t robId = 0;
    Instruction instruction;
    std::uint32_t src1Value = 0;
    std::uint32_t src2Value = 0;
    std::uint32_t effectiveAddress = 0;
    bool hasForwardedLoadValue = false;
    std::uint32_t forwardedLoadValue = 0;
};

class FunctionalUnit {
public:
    FunctionalUnit(FunctionalUnitType type, int latencyCycles);

    FunctionalUnitType type() const;
    bool busy() const;
    int remainingCycles() const;

    void issue(const IssuedOperation& operation);
    std::optional<IssuedOperation> tick();
    void clear();

private:
    FunctionalUnitType type_;
    int latencyCycles_;
    int remainingCycles_ = 0;
    std::optional<IssuedOperation> operation_;
};

FunctionalUnitType unitTypeFor(const Instruction& instruction);

} // namespace ooo
