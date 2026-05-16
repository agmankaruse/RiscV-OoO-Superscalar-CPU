#include "branch_predictor.hpp"

namespace ooo {

BranchPredictor::BranchPredictor(BranchPredictorType type) : type_(type) {}

void BranchPredictor::configure(BranchPredictorType type) {
    type_ = type;
    reset();
}

void BranchPredictor::reset() {
    returnAddressStack_.clear();
    oneBitTable_.clear();
    twoBitTable_.clear();
    branchTargetBuffer_.clear();
    totalPredictions_ = 0;
    correctPredictions_ = 0;
    mispredictions_ = 0;
}

int BranchPredictor::predictNextPc(const Instruction& instruction) const {
    const auto sequential = static_cast<int>(instruction.pc) + 1;
    if (!instruction.isControl()) {
        return sequential;
    }

    const auto btb = branchTargetBuffer_.find(instruction.pc);
    if (type_ == BranchPredictorType::Btb && btb != branchTargetBuffer_.end()) {
        return btb->second;
    }

    if (type_ == BranchPredictorType::ReturnStack && instruction.op == Opcode::JALR && !returnAddressStack_.empty()) {
        return returnAddressStack_.back();
    }

    if (type_ == BranchPredictorType::StaticTaken) {
        return instruction.target >= 0 ? instruction.target : sequential;
    }

    if (type_ == BranchPredictorType::OneBit) {
        const auto found = oneBitTable_.find(instruction.pc);
        const bool taken = found != oneBitTable_.end() && found->second;
        return taken && instruction.target >= 0 ? instruction.target : sequential;
    }

    if (type_ == BranchPredictorType::TwoBit) {
        const auto found = twoBitTable_.find(instruction.pc);
        const auto counter = found == twoBitTable_.end() ? kWeaklyNotTaken : found->second;
        const bool taken = counter >= 2;
        return taken && instruction.target >= 0 ? instruction.target : sequential;
    }

    return sequential;
}

void BranchPredictor::record(const Instruction& instruction, bool taken, int actualNextPc, bool mispredicted) {
    ++totalPredictions_;
    if (mispredicted) {
        ++mispredictions_;
    } else {
        ++correctPredictions_;
    }

    if (taken) {
        branchTargetBuffer_[instruction.pc] = actualNextPc;
    }

    if (instruction.op == Opcode::JAL && (instruction.rd == 1 || instruction.rd == 5)) {
        returnAddressStack_.push_back(static_cast<int>(instruction.pc) + 1);
        if (returnAddressStack_.size() > 16) {
            returnAddressStack_.erase(returnAddressStack_.begin());
        }
    } else if (instruction.op == Opcode::JALR && (instruction.rs1 == 1 || instruction.rs1 == 5) &&
               !returnAddressStack_.empty()) {
        returnAddressStack_.pop_back();
    }

    if (type_ == BranchPredictorType::OneBit) {
        oneBitTable_[instruction.pc] = taken;
    } else if (type_ == BranchPredictorType::TwoBit) {
        auto& counter = twoBitTable_[instruction.pc];
        if (counter == 0) {
            counter = kWeaklyNotTaken;
        }
        if (taken && counter < 3) {
            ++counter;
        } else if (!taken && counter > 0) {
            --counter;
        }
    }
}

BranchPredictorType BranchPredictor::type() const {
    return type_;
}

std::uint64_t BranchPredictor::totalPredictions() const {
    return totalPredictions_;
}

std::uint64_t BranchPredictor::correctPredictions() const {
    return correctPredictions_;
}

std::uint64_t BranchPredictor::mispredictions() const {
    return mispredictions_;
}

double BranchPredictor::accuracy() const {
    if (totalPredictions_ == 0) {
        return 0.0;
    }
    return static_cast<double>(correctPredictions_) / static_cast<double>(totalPredictions_);
}

} // namespace ooo
