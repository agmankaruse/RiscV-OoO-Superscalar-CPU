#include "branch_predictor.hpp"

namespace ooo {

int BranchPredictor::predictNextPc(const Instruction& instruction) const {
    return static_cast<int>(instruction.pc) + 1;
}

void BranchPredictor::record(const Instruction&, bool, bool) {
    // Static not-taken has no mutable state.
}

} // namespace ooo
