#pragma once

#include "instruction.hpp"
#include "rename_table.hpp"

#include <array>
#include <cstddef>
#include <deque>

namespace ooo {

struct RenamedInstruction {
    Instruction instruction;
    int src1Physical = -1;
    int src2Physical = -1;

    bool writesRegister = false;
    int architecturalDestination = -1;
    int physicalDestination = -1;
    int oldPhysicalDestination = -1;

    int predictedNextPc = -1;
    bool hasRenameSnapshot = false;
    std::array<int, RenameTable::kArchitecturalRegisters> renameSnapshot{};
};

// Front-end macro-pipeline latches. The ROB/IQ/FUs model the later stages.
class PipelineQueues {
public:
    explicit PipelineQueues(std::size_t queueCapacity = 8);

    void clear();
    bool empty() const;

    bool fetchCanAccept() const;
    bool decodeCanAccept() const;
    bool renameCanAccept() const;

    std::size_t queueCapacity() const;

    std::deque<Instruction> fetch;
    std::deque<Instruction> decode;
    std::deque<RenamedInstruction> rename;

private:
    std::size_t queueCapacity_;
};

} // namespace ooo
