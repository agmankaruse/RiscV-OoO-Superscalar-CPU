#pragma once

#include "instruction.hpp"
#include "config.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ooo {

class BranchPredictor {
public:
    explicit BranchPredictor(BranchPredictorType type = BranchPredictorType::StaticNotTaken);

    void configure(BranchPredictorType type);
    void reset();

    int predictNextPc(const Instruction& instruction) const;
    void record(const Instruction& instruction, bool taken, int actualNextPc, bool mispredicted);

    BranchPredictorType type() const;
    std::uint64_t totalPredictions() const;
    std::uint64_t correctPredictions() const;
    std::uint64_t mispredictions() const;
    double accuracy() const;

private:
    static constexpr std::uint8_t kWeaklyNotTaken = 1;

    BranchPredictorType type_ = BranchPredictorType::StaticNotTaken;
    mutable std::vector<int> returnAddressStack_;
    std::unordered_map<std::uint32_t, bool> oneBitTable_;
    std::unordered_map<std::uint32_t, std::uint8_t> twoBitTable_;
    std::unordered_map<std::uint32_t, int> branchTargetBuffer_;
    std::uint64_t totalPredictions_ = 0;
    std::uint64_t correctPredictions_ = 0;
    std::uint64_t mispredictions_ = 0;
};

} // namespace ooo
