#pragma once

#include "config.hpp"
#include "cpu.hpp"
#include "instruction.hpp"
#include "memory.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ooo {

struct ReferenceCpuState {
    std::array<std::uint32_t, 32> registers{};
    Memory memory;
    std::size_t pc = 0;
    std::uint64_t retiredInstructions = 0;
};

class ReferenceCPU {
public:
    void reset();
    void loadProgram(const std::vector<Instruction>& program);
    void loadProgramText(const std::string& text);
    void loadProgramFromFile(const std::string& path);
    void step();
    void run(std::size_t maxInstructions = 100000);

    bool halted() const;
    std::uint32_t readArchitecturalRegister(int architecturalRegister) const;
    std::size_t finalPc() const;
    std::uint64_t retiredInstructions() const;
    Memory& memory();
    const Memory& memory() const;
    const ReferenceCpuState& state() const;

private:
    void writeRegister(int architecturalRegister, std::uint32_t value);

    std::vector<Instruction> program_;
    ReferenceCpuState state_;
};

struct CpuDiffMismatch {
    std::string name;
    std::uint32_t oooValue = 0;
    std::uint32_t referenceValue = 0;
};

struct CpuDiffResult {
    std::string programName;
    bool passed = false;
    std::vector<CpuDiffMismatch> mismatches;
    std::size_t oooFinalPc = 0;
    std::size_t referenceFinalPc = 0;
    std::uint64_t oooRetiredInstructions = 0;
    std::uint64_t referenceRetiredInstructions = 0;
    std::uint64_t cycles = 0;
};

CpuDiffResult runDifferentialTest(const std::vector<Instruction>& program,
                                  const std::string& programName,
                                  const CpuConfig& config = CpuConfig{});
CpuDiffResult runDifferentialTestFromFile(const std::string& path,
                                          const CpuConfig& config = CpuConfig{});
std::string formatDiffResult(const CpuDiffResult& result);

} // namespace ooo
