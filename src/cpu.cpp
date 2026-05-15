#include "cpu.hpp"

#include "isa.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ooo {
namespace {

std::uint32_t shamt(std::uint32_t value) {
    return value & 0x1fu;
}

std::uint32_t pcToByteAddress(std::uint32_t pc) {
    return pc * 4u;
}

bool isSignedLess(std::uint32_t lhs, std::uint32_t rhs) {
    return static_cast<std::int32_t>(lhs) < static_cast<std::int32_t>(rhs);
}

bool isSignedGreaterEqual(std::uint32_t lhs, std::uint32_t rhs) {
    return static_cast<std::int32_t>(lhs) >= static_cast<std::int32_t>(rhs);
}

} // namespace

CPU::CPU(const CpuConfig& config)
    : config_(config),
      pipeline_(8),
      renameTable_(config.physicalRegisters),
      registerFile_(config.physicalRegisters),
      reorderBuffer_(config.robEntries),
      issueQueue_(config.issueQueueEntries),
      loadStoreQueue_(config.loadStoreQueueEntries) {
    functionalUnits_.emplace_back(FunctionalUnitType::Integer, 1);
    functionalUnits_.emplace_back(FunctionalUnitType::Integer, 1);
    functionalUnits_.emplace_back(FunctionalUnitType::Branch, 1);
    functionalUnits_.emplace_back(FunctionalUnitType::LoadStore, 2);
    reset();
}

void CPU::reset() {
    fetchPc_ = 0;
    nextRobId_ = 1;
    pipeline_.clear();
    renameTable_.reset();
    registerFile_.reset();
    reorderBuffer_.clear();
    issueQueue_.clear();
    loadStoreQueue_.clear();
    memory_.clear();
    stats_ = Stats{};
    for (auto& unit : functionalUnits_) {
        unit.clear();
    }
}

void CPU::loadProgram(const std::vector<Instruction>& program) {
    reset();
    program_ = program;
}

void CPU::loadProgramText(const std::string& text) {
    loadProgram(parseProgramText(text));
}

void CPU::loadProgramFromFile(const std::string& path) {
    loadProgram(parseProgramFile(path));
}

void CPU::setTrace(bool enabled) {
    traceEnabled_ = enabled;
}

void CPU::tick() {
    ++stats_.cycles;
    if (traceEnabled_) {
        std::cout << "cycle " << stats_.cycles << "\n";
    }

    writebackStage();
    commitStage();
    issueStage();
    dispatchStage();
    renameStage();
    decodeStage();
    fetchStage();

    stats_.observeOccupancy(reorderBuffer_.size(), issueQueue_.size(), loadStoreQueue_.size());
}

void CPU::run(std::size_t maxCycles) {
    while (!halted()) {
        if (stats_.cycles >= maxCycles) {
            throw std::runtime_error("simulation did not halt before max cycle limit");
        }
        tick();
    }
}

bool CPU::halted() const {
    const bool frontEndDrained = fetchPc_ >= program_.size() && pipeline_.empty();
    const bool backEndDrained = reorderBuffer_.empty() && issueQueue_.empty() && loadStoreQueue_.empty();
    const bool unitsIdle = std::all_of(functionalUnits_.begin(), functionalUnits_.end(), [](const FunctionalUnit& unit) {
        return !unit.busy();
    });
    return frontEndDrained && backEndDrained && unitsIdle;
}

std::uint32_t CPU::readArchitecturalRegister(int architecturalRegister) const {
    if (architecturalRegister == 0) {
        return 0;
    }
    return registerFile_.read(renameTable_.committedMapping(architecturalRegister));
}

Memory& CPU::memory() {
    return memory_;
}

const Memory& CPU::memory() const {
    return memory_;
}

const Stats& CPU::stats() const {
    return stats_;
}

void CPU::writebackStage() {
    for (auto& unit : functionalUnits_) {
        auto completed = unit.tick();
        if (completed) {
            completeOperation(*completed);
        }
    }
}

void CPU::commitStage() {
    int committed = 0;
    while (committed < config_.commitWidth && !reorderBuffer_.empty()) {
        auto& head = reorderBuffer_.head();
        if (!head.ready) {
            break;
        }

        if (head.instruction.isStore()) {
            memory_.writeWord(head.storeAddress, head.storeValue);
            loadStoreQueue_.retire(head.id);
            trace("commit store ROB" + std::to_string(head.id) + " mem[" +
                  std::to_string(head.storeAddress) + "]=" + std::to_string(head.storeValue));
        } else if (head.instruction.isLoad()) {
            loadStoreQueue_.retire(head.id);
        }

        if (head.writesRegister) {
            renameTable_.commit(head.architecturalDestination, head.physicalDestination);
            renameTable_.rebuildFreeList([&]() {
                std::set<int> used;
                for (int arch = 0; arch < RenameTable::kArchitecturalRegisters; ++arch) {
                    used.insert(renameTable_.currentMapping(arch));
                    used.insert(renameTable_.committedMapping(arch));
                }
                for (const auto& entry : reorderBuffer_.entries()) {
                    if (entry.writesRegister && entry.id != head.id) {
                        used.insert(entry.physicalDestination);
                        used.insert(entry.oldPhysicalDestination);
                    }
                }
                return used;
            }());
        }

        trace("commit ROB" + std::to_string(head.id) + " " + formatInstruction(head.instruction));
        reorderBuffer_.popHead();
        ++stats_.retiredInstructions;
        ++committed;
    }
}

void CPU::issueStage() {
    std::vector<std::uint64_t> issuedIds;

    for (const auto& entry : issueQueue_.entries()) {
        FunctionalUnit* selectedUnit = nullptr;
        for (auto& unit : functionalUnits_) {
            if (!unit.busy() && unit.type() == unitTypeFor(entry.instruction)) {
                selectedUnit = &unit;
                break;
            }
        }
        if (!selectedUnit || !operandsReady(entry)) {
            continue;
        }

        LoadIssueInfo loadInfo;
        if (entry.instruction.isLoad()) {
            const auto base = registerFile_.read(entry.src1Physical);
            const auto address = base + static_cast<std::uint32_t>(entry.instruction.imm);
            loadInfo = loadStoreQueue_.canLoadIssue(entry.robId, address);
            if (!loadInfo.canIssue) {
                continue;
            }
        }

        auto operation = buildIssuedOperation(entry, entry.instruction.isLoad() ? &loadInfo : nullptr);
        selectedUnit->issue(operation);
        issuedIds.push_back(entry.robId);
        trace("issue ROB" + std::to_string(entry.robId) + " " + formatInstruction(entry.instruction));
    }

    for (const auto id : issuedIds) {
        issueQueue_.remove(id);
    }
}

void CPU::dispatchStage() {
    int dispatched = 0;
    while (dispatched < config_.dispatchWidth && !pipeline_.rename.empty()) {
        const auto& renamed = pipeline_.rename.front();
        if (!reorderBuffer_.canAllocate() || !issueQueue_.canAllocate()) {
            break;
        }
        if (renamed.instruction.isMemory() && !loadStoreQueue_.canAllocate()) {
            break;
        }

        RobEntry robEntry;
        robEntry.id = nextRobId_++;
        robEntry.instruction = renamed.instruction;
        robEntry.writesRegister = renamed.writesRegister;
        robEntry.architecturalDestination = renamed.architecturalDestination;
        robEntry.physicalDestination = renamed.physicalDestination;
        robEntry.oldPhysicalDestination = renamed.oldPhysicalDestination;
        robEntry.predictedNextPc = renamed.predictedNextPc;
        robEntry.hasRenameSnapshot = renamed.hasRenameSnapshot;
        robEntry.renameSnapshot = renamed.renameSnapshot;

        const auto robId = reorderBuffer_.allocate(robEntry);

        if (renamed.instruction.isLoad()) {
            loadStoreQueue_.addLoad(robId);
        } else if (renamed.instruction.isStore()) {
            loadStoreQueue_.addStore(robId);
        }

        issueQueue_.add(IssueEntry{
            robId,
            renamed.instruction,
            renamed.src1Physical,
            renamed.src2Physical,
            renamed.physicalDestination
        });

        trace("dispatch ROB" + std::to_string(robId) + " " + formatInstruction(renamed.instruction));
        pipeline_.rename.pop_front();
        ++dispatched;
    }
}

void CPU::renameStage() {
    int renamedCount = 0;
    while (renamedCount < config_.renameWidth && !pipeline_.decode.empty() && pipeline_.renameCanAccept()) {
        const auto instruction = pipeline_.decode.front();
        if (instruction.writesRegister() && !renameTable_.canAllocate()) {
            break;
        }

        RenamedInstruction renamed;
        renamed.instruction = instruction;
        renamed.src1Physical = instruction.usesRs1() ? renameTable_.currentMapping(instruction.rs1) : -1;
        renamed.src2Physical = instruction.usesRs2() ? renameTable_.currentMapping(instruction.rs2) : -1;
        renamed.predictedNextPc = branchPredictor_.predictNextPc(instruction);

        if (instruction.writesRegister()) {
            const auto destination = renameTable_.allocateDestination(instruction.rd, registerFile_);
            renamed.writesRegister = destination.writes;
            renamed.architecturalDestination = destination.architecturalRegister;
            renamed.physicalDestination = destination.newPhysicalRegister;
            renamed.oldPhysicalDestination = destination.oldPhysicalRegister;
        }

        if (instruction.isControl()) {
            renamed.hasRenameSnapshot = true;
            renamed.renameSnapshot = renameTable_.snapshot();
        }

        pipeline_.decode.pop_front();
        pipeline_.rename.push_back(renamed);
        trace("rename " + formatInstruction(instruction));
        ++renamedCount;
    }
}

void CPU::decodeStage() {
    int decoded = 0;
    while (decoded < config_.decodeWidth && !pipeline_.fetch.empty() && pipeline_.decodeCanAccept()) {
        const auto instruction = pipeline_.fetch.front();
        pipeline_.fetch.pop_front();
        pipeline_.decode.push_back(instruction);
        trace("decode " + formatInstruction(instruction));
        ++decoded;
    }
}

void CPU::fetchStage() {
    int fetched = 0;
    while (fetched < config_.fetchWidth && pipeline_.fetchCanAccept() && fetchPc_ < program_.size()) {
        const auto instruction = program_[fetchPc_];
        pipeline_.fetch.push_back(instruction);
        trace("fetch pc=" + std::to_string(fetchPc_) + " " + formatInstruction(instruction));
        ++fetchPc_;
        ++fetched;
    }
}

bool CPU::operandsReady(const IssueEntry& entry) const {
    if (entry.instruction.usesRs1() && !registerFile_.isReady(entry.src1Physical)) {
        return false;
    }
    if (entry.instruction.usesRs2() && !registerFile_.isReady(entry.src2Physical)) {
        return false;
    }
    return true;
}

IssuedOperation CPU::buildIssuedOperation(const IssueEntry& entry, const LoadIssueInfo* loadInfo) const {
    IssuedOperation operation;
    operation.robId = entry.robId;
    operation.instruction = entry.instruction;
    operation.src1Value = entry.instruction.usesRs1() ? registerFile_.read(entry.src1Physical) : 0;
    operation.src2Value = entry.instruction.usesRs2() ? registerFile_.read(entry.src2Physical) : 0;

    if (entry.instruction.isMemory()) {
        operation.effectiveAddress = operation.src1Value + static_cast<std::uint32_t>(entry.instruction.imm);
    }
    if (loadInfo && loadInfo->hasForwardedValue) {
        operation.hasForwardedLoadValue = true;
        operation.forwardedLoadValue = loadInfo->forwardedValue;
    }

    return operation;
}

void CPU::completeOperation(const IssuedOperation& operation) {
    auto* robEntry = reorderBuffer_.find(operation.robId);
    if (!robEntry) {
        return;
    }

    const auto& instruction = operation.instruction;
    std::uint32_t result = 0;
    bool hasResult = instruction.writesRegister();
    bool branchTaken = false;
    int actualNextPc = static_cast<int>(instruction.pc) + 1;

    switch (instruction.op) {
    case Opcode::ADD:
        result = operation.src1Value + operation.src2Value;
        break;
    case Opcode::SUB:
        result = operation.src1Value - operation.src2Value;
        break;
    case Opcode::AND:
        result = operation.src1Value & operation.src2Value;
        break;
    case Opcode::OR:
        result = operation.src1Value | operation.src2Value;
        break;
    case Opcode::XOR:
        result = operation.src1Value ^ operation.src2Value;
        break;
    case Opcode::SLL:
        result = operation.src1Value << shamt(operation.src2Value);
        break;
    case Opcode::SRL:
        result = operation.src1Value >> shamt(operation.src2Value);
        break;
    case Opcode::SRA:
        result = static_cast<std::uint32_t>(static_cast<std::int32_t>(operation.src1Value) >> shamt(operation.src2Value));
        break;
    case Opcode::ADDI:
        result = operation.src1Value + static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::ANDI:
        result = operation.src1Value & static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::ORI:
        result = operation.src1Value | static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::XORI:
        result = operation.src1Value ^ static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::LW:
        result = operation.hasForwardedLoadValue
            ? operation.forwardedLoadValue
            : memory_.readWord(operation.effectiveAddress);
        loadStoreQueue_.markLoadComplete(operation.robId, operation.effectiveAddress, result);
        break;
    case Opcode::SW:
        hasResult = false;
        robEntry->storeAddress = operation.effectiveAddress;
        robEntry->storeValue = operation.src2Value;
        loadStoreQueue_.markStoreReady(operation.robId, operation.effectiveAddress, operation.src2Value);
        break;
    case Opcode::BEQ:
        hasResult = false;
        branchTaken = operation.src1Value == operation.src2Value;
        actualNextPc = branchTaken ? instruction.target : static_cast<int>(instruction.pc) + 1;
        break;
    case Opcode::BNE:
        hasResult = false;
        branchTaken = operation.src1Value != operation.src2Value;
        actualNextPc = branchTaken ? instruction.target : static_cast<int>(instruction.pc) + 1;
        break;
    case Opcode::BLT:
        hasResult = false;
        branchTaken = isSignedLess(operation.src1Value, operation.src2Value);
        actualNextPc = branchTaken ? instruction.target : static_cast<int>(instruction.pc) + 1;
        break;
    case Opcode::BGE:
        hasResult = false;
        branchTaken = isSignedGreaterEqual(operation.src1Value, operation.src2Value);
        actualNextPc = branchTaken ? instruction.target : static_cast<int>(instruction.pc) + 1;
        break;
    case Opcode::JAL:
        branchTaken = true;
        result = pcToByteAddress(instruction.pc + 1);
        actualNextPc = instruction.target;
        break;
    case Opcode::JALR:
        branchTaken = true;
        result = pcToByteAddress(instruction.pc + 1);
        actualNextPc = static_cast<int>(((operation.src1Value + static_cast<std::uint32_t>(instruction.imm)) & ~1u) / 4u);
        break;
    case Opcode::LUI:
        result = static_cast<std::uint32_t>(instruction.imm) << 12u;
        break;
    case Opcode::AUIPC:
        result = pcToByteAddress(instruction.pc) + (static_cast<std::uint32_t>(instruction.imm) << 12u);
        break;
    case Opcode::NOP:
        hasResult = false;
        break;
    }

    if (hasResult && robEntry->writesRegister) {
        robEntry->value = result;
        registerFile_.write(robEntry->physicalDestination, result);
    }

    robEntry->ready = true;
    trace("writeback ROB" + std::to_string(operation.robId) + " " + formatInstruction(instruction));

    if (instruction.isControl()) {
        const bool mispredicted = actualNextPc != robEntry->predictedNextPc;
        branchPredictor_.record(instruction, branchTaken, mispredicted);
        if (mispredicted) {
            ++stats_.branchMispredicts;
            trace("branch recovery ROB" + std::to_string(operation.robId) +
                  " redirect pc=" + std::to_string(actualNextPc));
            recoverFromBranch(operation.robId, actualNextPc);
        }
    }
}

void CPU::recoverFromBranch(std::uint64_t robId, int actualNextPc) {
    auto* branch = reorderBuffer_.find(robId);
    if (!branch || !branch->hasRenameSnapshot) {
        throw std::runtime_error("branch recovery missing rename snapshot");
    }
    const auto renameSnapshot = branch->renameSnapshot;

    pipeline_.clear();
    issueQueue_.flushYoungerThan(robId);
    loadStoreQueue_.flushYoungerThan(robId);
    reorderBuffer_.flushYoungerThan(robId);
    renameTable_.restore(renameSnapshot);
    rebuildFreeListAfterRecovery();

    fetchPc_ = actualNextPc < 0 ? program_.size() : static_cast<std::size_t>(actualNextPc);
}

void CPU::rebuildFreeListAfterRecovery() {
    std::set<int> used;
    used.insert(0);
    for (int arch = 0; arch < RenameTable::kArchitecturalRegisters; ++arch) {
        used.insert(renameTable_.currentMapping(arch));
        used.insert(renameTable_.committedMapping(arch));
    }
    for (const auto& entry : reorderBuffer_.entries()) {
        if (entry.writesRegister) {
            used.insert(entry.physicalDestination);
            used.insert(entry.oldPhysicalDestination);
        }
    }
    renameTable_.rebuildFreeList(used);
}

void CPU::trace(const std::string& message) const {
    if (traceEnabled_) {
        std::cout << "  " << message << "\n";
    }
}

} // namespace ooo
