#pragma once

#include <cstdint>
#include <string>

namespace ooo {

enum class Opcode {
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    SLL,
    SRL,
    SRA,
    MUL,
    MULH,
    DIV,
    REM,
    ADDI,
    ANDI,
    ORI,
    XORI,
    LW,
    SW,
    BEQ,
    BNE,
    BLT,
    BGE,
    JAL,
    JALR,
    LUI,
    AUIPC,
    NOP
};

struct Instruction {
    Opcode op = Opcode::NOP;
    int rd = -1;
    int rs1 = -1;
    int rs2 = -1;
    std::int32_t imm = 0;
    int target = -1;
    int predictedNextPc = -1;
    std::uint32_t pc = 0;
    std::uint64_t dynamicId = 0;
    std::string text;

    bool usesRs1() const;
    bool usesRs2() const;
    bool writesRegister() const;
    bool isLoad() const;
    bool isStore() const;
    bool isMemory() const;
    bool isMultiplyDivide() const;
    bool isBranch() const;
    bool isJump() const;
    bool isControl() const;
};

std::string opcodeName(Opcode op);

} // namespace ooo
