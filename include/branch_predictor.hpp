#pragma once

#include "instruction.hpp"

namespace ooo {

// First predictor: static not-taken. It is deliberately small so students can
// replace it with a BTB, BHT, or tournament predictor later.
class BranchPredictor {
public:
    int predictNextPc(const Instruction& instruction) const;
    void record(const Instruction& instruction, bool taken, bool mispredicted);
};

} // namespace ooo
