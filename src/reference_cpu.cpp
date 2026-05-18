#include "reference_cpu.hpp"

#include "isa.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace ooo {
namespace {

std::uint32_t pcToByteAddress(std::uint32_t pc) {
    return pc * 4u;
}

std::uint32_t shamt(std::uint32_t value) {
    return value & 0x1fu;
}

bool isSignedLess(std::uint32_t lhs, std::uint32_t rhs) {
    return static_cast<std::int32_t>(lhs) < static_cast<std::int32_t>(rhs);
}

bool isSignedGreaterEqual(std::uint32_t lhs, std::uint32_t rhs) {
    return static_cast<std::int32_t>(lhs) >= static_cast<std::int32_t>(rhs);
}

std::map<std::uint32_t, std::uint32_t> memoryMap(const Memory& memory) {
    std::map<std::uint32_t, std::uint32_t> result;
    for (const auto& [address, value] : memory.words()) {
        result[address] = value;
    }
    return result;
}

} // namespace

void ReferenceCPU::reset() {
    state_ = ReferenceCpuState{};
    program_.clear();
}

void ReferenceCPU::loadProgram(const std::vector<Instruction>& program) {
    reset();
    program_ = program;
}

void ReferenceCPU::loadProgramText(const std::string& text) {
    loadProgram(parseProgramText(text));
}

void ReferenceCPU::loadProgramFromFile(const std::string& path) {
    loadProgram(parseProgramFile(path));
}

void ReferenceCPU::step() {
    if (halted()) {
        return;
    }

    const auto& instruction = program_.at(state_.pc);
    std::uint32_t result = 0;
    bool writesRegister = instruction.writesRegister();
    std::size_t nextPc = state_.pc + 1;

    const auto rs1 = instruction.usesRs1() ? state_.registers.at(static_cast<std::size_t>(instruction.rs1)) : 0;
    const auto rs2 = instruction.usesRs2() ? state_.registers.at(static_cast<std::size_t>(instruction.rs2)) : 0;

    switch (instruction.op) {
    case Opcode::ADD:
        result = rs1 + rs2;
        break;
    case Opcode::SUB:
        result = rs1 - rs2;
        break;
    case Opcode::AND:
        result = rs1 & rs2;
        break;
    case Opcode::OR:
        result = rs1 | rs2;
        break;
    case Opcode::XOR:
        result = rs1 ^ rs2;
        break;
    case Opcode::SLL:
        result = rs1 << shamt(rs2);
        break;
    case Opcode::SRL:
        result = rs1 >> shamt(rs2);
        break;
    case Opcode::SRA:
        result = static_cast<std::uint32_t>(static_cast<std::int32_t>(rs1) >> shamt(rs2));
        break;
    case Opcode::MUL:
        result = static_cast<std::uint32_t>(
            static_cast<std::int64_t>(static_cast<std::int32_t>(rs1)) *
            static_cast<std::int64_t>(static_cast<std::int32_t>(rs2)));
        break;
    case Opcode::MULH:
        result = static_cast<std::uint32_t>((
            static_cast<std::int64_t>(static_cast<std::int32_t>(rs1)) *
            static_cast<std::int64_t>(static_cast<std::int32_t>(rs2))) >> 32);
        break;
    case Opcode::DIV:
        if (rs2 == 0) {
            result = 0xffffffffu;
        } else if (rs1 == 0x80000000u && rs2 == 0xffffffffu) {
            result = 0x80000000u;
        } else {
            result = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(rs1) / static_cast<std::int32_t>(rs2));
        }
        break;
    case Opcode::REM:
        if (rs2 == 0) {
            result = rs1;
        } else if (rs1 == 0x80000000u && rs2 == 0xffffffffu) {
            result = 0;
        } else {
            result = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(rs1) % static_cast<std::int32_t>(rs2));
        }
        break;
    case Opcode::ADDI:
        result = rs1 + static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::ANDI:
        result = rs1 & static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::ORI:
        result = rs1 | static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::XORI:
        result = rs1 ^ static_cast<std::uint32_t>(instruction.imm);
        break;
    case Opcode::LW:
        result = state_.memory.readWord(rs1 + static_cast<std::uint32_t>(instruction.imm));
        break;
    case Opcode::SW:
        writesRegister = false;
        state_.memory.writeWord(rs1 + static_cast<std::uint32_t>(instruction.imm), rs2);
        break;
    case Opcode::BEQ:
        writesRegister = false;
        nextPc = (rs1 == rs2) ? static_cast<std::size_t>(instruction.target) : state_.pc + 1;
        break;
    case Opcode::BNE:
        writesRegister = false;
        nextPc = (rs1 != rs2) ? static_cast<std::size_t>(instruction.target) : state_.pc + 1;
        break;
    case Opcode::BLT:
        writesRegister = false;
        nextPc = isSignedLess(rs1, rs2) ? static_cast<std::size_t>(instruction.target) : state_.pc + 1;
        break;
    case Opcode::BGE:
        writesRegister = false;
        nextPc = isSignedGreaterEqual(rs1, rs2) ? static_cast<std::size_t>(instruction.target) : state_.pc + 1;
        break;
    case Opcode::JAL:
        result = pcToByteAddress(static_cast<std::uint32_t>(state_.pc + 1));
        nextPc = static_cast<std::size_t>(instruction.target);
        break;
    case Opcode::JALR:
        result = pcToByteAddress(static_cast<std::uint32_t>(state_.pc + 1));
        nextPc = static_cast<std::size_t>(((rs1 + static_cast<std::uint32_t>(instruction.imm)) & ~1u) / 4u);
        break;
    case Opcode::LUI:
        result = static_cast<std::uint32_t>(instruction.imm) << 12u;
        break;
    case Opcode::AUIPC:
        result = pcToByteAddress(static_cast<std::uint32_t>(state_.pc)) +
                 (static_cast<std::uint32_t>(instruction.imm) << 12u);
        break;
    case Opcode::NOP:
        writesRegister = false;
        break;
    }

    if (writesRegister) {
        writeRegister(instruction.rd, result);
    }
    state_.registers[0] = 0;
    state_.pc = nextPc;
    ++state_.retiredInstructions;
}

void ReferenceCPU::run(std::size_t maxInstructions) {
    while (!halted()) {
        if (state_.retiredInstructions >= maxInstructions) {
            throw std::runtime_error("reference CPU did not halt before instruction limit");
        }
        step();
    }
}

bool ReferenceCPU::halted() const {
    return state_.pc >= program_.size();
}

std::uint32_t ReferenceCPU::readArchitecturalRegister(int architecturalRegister) const {
    if (architecturalRegister == 0) {
        return 0;
    }
    return state_.registers.at(static_cast<std::size_t>(architecturalRegister));
}

std::size_t ReferenceCPU::finalPc() const {
    return state_.pc;
}

std::uint64_t ReferenceCPU::retiredInstructions() const {
    return state_.retiredInstructions;
}

Memory& ReferenceCPU::memory() {
    return state_.memory;
}

const Memory& ReferenceCPU::memory() const {
    return state_.memory;
}

const ReferenceCpuState& ReferenceCPU::state() const {
    return state_;
}

void ReferenceCPU::writeRegister(int architecturalRegister, std::uint32_t value) {
    if (architecturalRegister <= 0) {
        state_.registers[0] = 0;
        return;
    }
    state_.registers.at(static_cast<std::size_t>(architecturalRegister)) = value;
}

CpuDiffResult runDifferentialTest(const std::vector<Instruction>& program,
                                  const std::string& programName,
                                  const CpuConfig& config) {
    CPU cpu(config);
    ReferenceCPU reference;
    cpu.loadProgram(program);
    reference.loadProgram(program);

    cpu.run();
    reference.run();

    CpuDiffResult result;
    result.programName = programName;
    result.oooFinalPc = cpu.finalPc();
    result.referenceFinalPc = reference.finalPc();
    result.oooRetiredInstructions = cpu.stats().retiredInstructions;
    result.referenceRetiredInstructions = reference.retiredInstructions();
    result.cycles = cpu.stats().cycles;

    for (int reg = 0; reg < 32; ++reg) {
        const auto oooValue = cpu.readArchitecturalRegister(reg);
        const auto referenceValue = reference.readArchitecturalRegister(reg);
        if (oooValue != referenceValue) {
            result.mismatches.push_back({"x" + std::to_string(reg), oooValue, referenceValue});
        }
    }

    auto oooMemory = memoryMap(cpu.memory());
    const auto referenceMemory = memoryMap(reference.memory());
    for (const auto& [address, value] : referenceMemory) {
        oooMemory.emplace(address, 0);
        (void)value;
    }
    for (const auto& [address, value] : oooMemory) {
        const auto referenceFound = referenceMemory.find(address);
        const auto referenceValue = referenceFound == referenceMemory.end() ? 0 : referenceFound->second;
        if (value != referenceValue) {
            result.mismatches.push_back({"mem[" + std::to_string(address) + "]", value, referenceValue});
        }
    }

    if (result.oooFinalPc != result.referenceFinalPc) {
        result.mismatches.push_back({"pc",
                                     static_cast<std::uint32_t>(result.oooFinalPc),
                                     static_cast<std::uint32_t>(result.referenceFinalPc)});
    }

    result.passed = result.mismatches.empty();
    return result;
}

CpuDiffResult runDifferentialTestFromFile(const std::string& path, const CpuConfig& config) {
    return runDifferentialTest(parseProgramFile(path), path, config);
}

std::string formatDiffResult(const CpuDiffResult& result) {
    std::ostringstream out;
    out << (result.passed ? "PASS" : "FAIL") << " diff program=" << result.programName
        << " cycles=" << result.cycles
        << " ooo_final_pc=" << result.oooFinalPc
        << " reference_final_pc=" << result.referenceFinalPc
        << " ooo_retired=" << result.oooRetiredInstructions
        << " reference_retired=" << result.referenceRetiredInstructions << '\n';

    for (const auto& mismatch : result.mismatches) {
        out << "mismatch " << mismatch.name
            << " ooo=" << mismatch.oooValue
            << " reference=" << mismatch.referenceValue << '\n';
    }
    return out.str();
}

} // namespace ooo
