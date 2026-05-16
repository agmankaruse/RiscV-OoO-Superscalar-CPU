#include "cpu.hpp"

#include "isa.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
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
    branchPredictor_.configure(config_.branchPredictorType);
    instructionCache_.configure(config_.l1InstructionCacheSize, config_.cacheLineSize,
                                config_.l1CacheHitLatency, config_.l1CacheMissLatency);
    dataCache_.configure(config_.l1DataCacheSize, config_.cacheLineSize,
                         config_.l1CacheHitLatency, config_.l1CacheMissLatency);
    functionalUnits_.emplace_back(FunctionalUnitType::Integer, config_.aluLatency);
    functionalUnits_.emplace_back(FunctionalUnitType::Integer, config_.aluLatency);
    functionalUnits_.emplace_back(FunctionalUnitType::Branch, config_.branchLatency);
    functionalUnits_.emplace_back(FunctionalUnitType::LoadStore, config_.l1CacheHitLatency);
    reset();
}

void CPU::reset() {
    fetchPc_ = 0;
    fetchStallCycles_ = 0;
    nextRobId_ = 1;
    nextDynamicInstructionId_ = 1;
    pipeline_.clear();
    renameTable_.reset();
    registerFile_.reset();
    reorderBuffer_.clear();
    issueQueue_.clear();
    loadStoreQueue_.clear();
    branchPredictor_.reset();
    instructionCache_.reset();
    dataCache_.reset();
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

void CPU::enableTimelineCsv(const std::string& path) {
    timelineCsvPath_ = path;
    configureOutputFile(path, "cycle,instruction_id,pc,instruction,stage,event,rob_index,physical_dest\n");
}

void CPU::enableStatsCsv(const std::string& path) {
    statsCsvPath_ = path;
    configureOutputFile(path, "cycle,rob_occupancy,iq_occupancy,lsq_occupancy,free_phys_regs,committed,inflight\n");
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
    recordStatsCsvRow();
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
        recordTimeline(head.instruction, "Commit / Retire", "COMMIT", head.id, head.physicalDestination);
        reorderBuffer_.popHead();
        ++stats_.retiredInstructions;
        ++committed;
    }
}

void CPU::issueStage() {
    std::vector<std::uint64_t> issuedIds;
    int issuedThisCycle = 0;

    for (const auto& entry : issueQueue_.entries()) {
        if (issuedThisCycle >= config_.issueWidth) {
            break;
        }
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
        if (entry.instruction.isLoad()) {
            if (loadInfo.hasForwardedValue) {
                operation.latencyCycles = config_.l1CacheHitLatency;
            } else {
                const auto cache = dataCache_.access(operation.effectiveAddress);
                operation.latencyCycles = cache.latencyCycles;
                if (cache.hit) {
                    ++stats_.dataCacheHits;
                } else {
                    ++stats_.dataCacheMisses;
                    stats_.loadMissStalls += static_cast<std::uint64_t>(std::max(0, cache.latencyCycles - 1));
                }
            }
        } else if (entry.instruction.isStore()) {
            const auto cache = dataCache_.access(operation.effectiveAddress);
            operation.latencyCycles = cache.latencyCycles;
            if (cache.hit) {
                ++stats_.dataCacheHits;
            } else {
                ++stats_.dataCacheMisses;
                stats_.loadMissStalls += static_cast<std::uint64_t>(std::max(0, cache.latencyCycles - 1));
            }
        }
        selectedUnit->issue(operation);
        issuedIds.push_back(entry.robId);
        ++issuedThisCycle;
        trace("issue ROB" + std::to_string(entry.robId) + " " + formatInstruction(entry.instruction));
        recordTimeline(entry.instruction, "Issue / Select", "ISSUE", entry.robId, entry.destPhysical);
        recordTimeline(entry.instruction, "Execute", "EXECUTE_START", entry.robId, entry.destPhysical);
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
        recordTimeline(renamed.instruction, "Dispatch", "DISPATCH", robId, renamed.physicalDestination);
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
        renamed.predictedNextPc = instruction.predictedNextPc >= 0
            ? instruction.predictedNextPc
            : branchPredictor_.predictNextPc(instruction);

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
        recordTimeline(instruction, "Rename", "RENAME", 0, renamed.physicalDestination);
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
        recordTimeline(instruction, "Decode", "DECODE");
        ++decoded;
    }
}

void CPU::fetchStage() {
    if (fetchStallCycles_ > 0) {
        --fetchStallCycles_;
        ++stats_.fetchMissStalls;
        trace("fetch stalled by I-cache miss");
        return;
    }

    int fetched = 0;
    while (fetched < config_.fetchWidth && pipeline_.fetchCanAccept() && fetchPc_ < program_.size()) {
        const auto cache = instructionCache_.access(pcToByteAddress(static_cast<std::uint32_t>(fetchPc_)));
        if (cache.hit) {
            ++stats_.instructionCacheHits;
        } else {
            ++stats_.instructionCacheMisses;
            fetchStallCycles_ = std::max(0, cache.latencyCycles - 1);
            ++stats_.fetchMissStalls;
            trace("I-cache miss at pc=" + std::to_string(fetchPc_));
            return;
        }

        auto instruction = program_[fetchPc_];
        instruction.dynamicId = nextDynamicInstructionId_++;
        instruction.predictedNextPc = branchPredictor_.predictNextPc(instruction);
        pipeline_.fetch.push_back(instruction);
        trace("fetch pc=" + std::to_string(fetchPc_) + " " + formatInstruction(instruction));
        recordTimeline(instruction, "Instruction Fetch", "FETCH");

        const auto sequential = static_cast<int>(instruction.pc) + 1;
        if (instruction.isControl() && instruction.predictedNextPc != sequential) {
            fetchPc_ = instruction.predictedNextPc < 0
                ? program_.size()
                : static_cast<std::size_t>(instruction.predictedNextPc);
            break;
        }

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
    operation.latencyCycles = latencyFor(entry.instruction);

    return operation;
}

int CPU::latencyFor(const Instruction& instruction) const {
    switch (instruction.op) {
    case Opcode::MUL:
    case Opcode::MULH:
        return config_.mulLatency;
    case Opcode::DIV:
    case Opcode::REM:
        return config_.divLatency;
    case Opcode::BEQ:
    case Opcode::BNE:
    case Opcode::BLT:
    case Opcode::BGE:
    case Opcode::JAL:
    case Opcode::JALR:
        return config_.branchLatency;
    case Opcode::LW:
    case Opcode::SW:
        return config_.l1CacheHitLatency;
    default:
        return config_.aluLatency;
    }
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
    case Opcode::MUL:
        result = static_cast<std::uint32_t>(
            static_cast<std::int64_t>(static_cast<std::int32_t>(operation.src1Value)) *
            static_cast<std::int64_t>(static_cast<std::int32_t>(operation.src2Value)));
        break;
    case Opcode::MULH:
        result = static_cast<std::uint32_t>((
            static_cast<std::int64_t>(static_cast<std::int32_t>(operation.src1Value)) *
            static_cast<std::int64_t>(static_cast<std::int32_t>(operation.src2Value))) >> 32);
        break;
    case Opcode::DIV:
        if (operation.src2Value == 0) {
            result = 0xffffffffu;
        } else if (operation.src1Value == 0x80000000u && operation.src2Value == 0xffffffffu) {
            result = 0x80000000u;
        } else {
            result = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(operation.src1Value) / static_cast<std::int32_t>(operation.src2Value));
        }
        break;
    case Opcode::REM:
        if (operation.src2Value == 0) {
            result = operation.src1Value;
        } else if (operation.src1Value == 0x80000000u && operation.src2Value == 0xffffffffu) {
            result = 0;
        } else {
            result = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(operation.src1Value) % static_cast<std::int32_t>(operation.src2Value));
        }
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
    recordTimeline(instruction, "Execute", "EXECUTE_DONE", operation.robId, robEntry->physicalDestination);
    trace("writeback ROB" + std::to_string(operation.robId) + " " + formatInstruction(instruction));
    recordTimeline(instruction, "Writeback", "WRITEBACK", operation.robId, robEntry->physicalDestination);

    if (instruction.isControl()) {
        const bool mispredicted = actualNextPc != robEntry->predictedNextPc;
        branchPredictor_.record(instruction, branchTaken, actualNextPc, mispredicted);
        ++stats_.branchPredictions;
        if (mispredicted) {
            ++stats_.branchMispredicts;
            stats_.mispredictionPenaltyCycles += static_cast<std::uint64_t>(config_.mispredictPenaltyCycles);
            trace("branch recovery ROB" + std::to_string(operation.robId) +
                  " redirect pc=" + std::to_string(actualNextPc));
            recoverFromBranch(operation.robId, actualNextPc);
        } else {
            ++stats_.correctBranchPredictions;
        }
    }
}

void CPU::recoverFromBranch(std::uint64_t robId, int actualNextPc) {
    auto* branch = reorderBuffer_.find(robId);
    if (!branch || !branch->hasRenameSnapshot) {
        throw std::runtime_error("branch recovery missing rename snapshot");
    }
    const auto renameSnapshot = branch->renameSnapshot;

    for (const auto& instruction : pipeline_.fetch) {
        recordTimeline(instruction, "Flush", "FLUSHED");
    }
    for (const auto& instruction : pipeline_.decode) {
        recordTimeline(instruction, "Flush", "FLUSHED");
    }
    for (const auto& renamed : pipeline_.rename) {
        recordTimeline(renamed.instruction, "Flush", "FLUSHED", 0, renamed.physicalDestination);
    }
    pipeline_.clear();
    issueQueue_.flushYoungerThan(robId);
    loadStoreQueue_.flushYoungerThan(robId);
    const auto flushed = reorderBuffer_.flushYoungerThan(robId);
    for (const auto& entry : flushed) {
        recordTimeline(entry.instruction, "Flush", "FLUSHED", entry.id, entry.physicalDestination);
    }
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

void CPU::configureOutputFile(const std::string& path, const std::string& header) {
    const std::filesystem::path outputPath(path);
    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    std::ofstream output(path, std::ios::trunc);
    output << header;
}

void CPU::recordTimeline(const Instruction& instruction,
                         const std::string& stage,
                         const std::string& event,
                         std::uint64_t robId,
                         int physicalDestination) {
    if (timelineCsvPath_.empty()) {
        return;
    }

    auto clean = [](std::string text) {
        std::replace(text.begin(), text.end(), ',', ' ');
        return text;
    };

    std::ofstream output(timelineCsvPath_, std::ios::app);
    output << stats_.cycles << ','
           << instruction.dynamicId << ','
           << instruction.pc << ','
           << clean(formatInstruction(instruction)) << ','
           << clean(stage) << ','
           << event << ','
           << robId << ','
           << physicalDestination << '\n';
}

void CPU::recordStatsCsvRow() {
    if (statsCsvPath_.empty()) {
        return;
    }

    std::ofstream output(statsCsvPath_, std::ios::app);
    output << stats_.cycles << ','
           << stats_.robOccupancy << ','
           << stats_.issueQueueOccupancy << ','
           << stats_.loadStoreQueueOccupancy << ','
           << renameTable_.freeCount() << ','
           << stats_.retiredInstructions << ','
           << reorderBuffer_.size() << '\n';
}

void CPU::trace(const std::string& message) const {
    if (traceEnabled_) {
        std::cout << "  " << message << "\n";
    }
}

} // namespace ooo
