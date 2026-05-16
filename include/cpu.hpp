#pragma once

#include "branch_predictor.hpp"
#include "cache.hpp"
#include "config.hpp"
#include "functional_unit.hpp"
#include "issue_queue.hpp"
#include "load_store_queue.hpp"
#include "memory.hpp"
#include "physical_register_file.hpp"
#include "pipeline.hpp"
#include "rename_table.hpp"
#include "reorder_buffer.hpp"
#include "stats.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ooo {

class CPU {
public:
    explicit CPU(const CpuConfig& config = CpuConfig{});

    void reset();
    void loadProgram(const std::vector<Instruction>& program);
    void loadProgramText(const std::string& text);
    void loadProgramFromFile(const std::string& path);

    void setTrace(bool enabled);
    void enableTimelineCsv(const std::string& path);
    void enableStatsCsv(const std::string& path);
    void tick();
    void run(std::size_t maxCycles = 100000);

    bool halted() const;

    std::uint32_t readArchitecturalRegister(int architecturalRegister) const;
    Memory& memory();
    const Memory& memory() const;
    const Stats& stats() const;

private:
    void writebackStage();
    void commitStage();
    void issueStage();
    void dispatchStage();
    void renameStage();
    void decodeStage();
    void fetchStage();

    bool operandsReady(const IssueEntry& entry) const;
    IssuedOperation buildIssuedOperation(const IssueEntry& entry, const LoadIssueInfo* loadInfo = nullptr) const;
    int latencyFor(const Instruction& instruction) const;
    void completeOperation(const IssuedOperation& operation);
    void recoverFromBranch(std::uint64_t robId, int actualNextPc);
    void rebuildFreeListAfterRecovery();
    void configureOutputFile(const std::string& path, const std::string& header);
    void recordTimeline(const Instruction& instruction,
                        const std::string& stage,
                        const std::string& event,
                        std::uint64_t robId = 0,
                        int physicalDestination = -1);
    void recordStatsCsvRow();
    void trace(const std::string& message) const;

    CpuConfig config_;
    std::vector<Instruction> program_;
    std::size_t fetchPc_ = 0;
    int fetchStallCycles_ = 0;
    std::uint64_t nextRobId_ = 1;
    std::uint64_t nextDynamicInstructionId_ = 1;
    bool traceEnabled_ = false;
    std::string timelineCsvPath_;
    std::string statsCsvPath_;

    PipelineQueues pipeline_;
    RenameTable renameTable_;
    PhysicalRegisterFile registerFile_;
    ReorderBuffer reorderBuffer_;
    IssueQueue issueQueue_;
    LoadStoreQueue loadStoreQueue_;
    BranchPredictor branchPredictor_;
    DirectMappedCache instructionCache_;
    DirectMappedCache dataCache_;
    Memory memory_;
    Stats stats_;

    std::vector<FunctionalUnit> functionalUnits_;
};

} // namespace ooo
