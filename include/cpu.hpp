#pragma once

#include "branch_predictor.hpp"
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

struct CpuConfig {
    int fetchWidth = 2;
    int decodeWidth = 2;
    int renameWidth = 2;
    int dispatchWidth = 2;
    int commitWidth = 2;

    std::size_t robEntries = 32;
    std::size_t issueQueueEntries = 16;
    std::size_t loadStoreQueueEntries = 16;
    int physicalRegisters = 64;
};

class CPU {
public:
    explicit CPU(const CpuConfig& config = CpuConfig{});

    void reset();
    void loadProgram(const std::vector<Instruction>& program);
    void loadProgramText(const std::string& text);
    void loadProgramFromFile(const std::string& path);

    void setTrace(bool enabled);
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
    void completeOperation(const IssuedOperation& operation);
    void recoverFromBranch(std::uint64_t robId, int actualNextPc);
    void rebuildFreeListAfterRecovery();
    void trace(const std::string& message) const;

    CpuConfig config_;
    std::vector<Instruction> program_;
    std::size_t fetchPc_ = 0;
    std::uint64_t nextRobId_ = 1;
    bool traceEnabled_ = false;

    PipelineQueues pipeline_;
    RenameTable renameTable_;
    PhysicalRegisterFile registerFile_;
    ReorderBuffer reorderBuffer_;
    IssueQueue issueQueue_;
    LoadStoreQueue loadStoreQueue_;
    BranchPredictor branchPredictor_;
    Memory memory_;
    Stats stats_;

    std::vector<FunctionalUnit> functionalUnits_;
};

} // namespace ooo
