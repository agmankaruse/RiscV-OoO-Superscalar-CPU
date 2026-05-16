#include "isa.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace ooo {
namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string stripComment(const std::string& line) {
    const auto hash = line.find('#');
    const auto semicolon = line.find(';');
    const auto slash = line.find("//");
    auto cut = std::min(hash == std::string::npos ? line.size() : hash,
                        semicolon == std::string::npos ? line.size() : semicolon);
    cut = std::min(cut, slash == std::string::npos ? line.size() : slash);
    return line.substr(0, cut);
}

std::string upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

std::vector<std::string> tokenize(std::string line) {
    for (char& c : line) {
        if (c == ',' || c == '(' || c == ')') {
            c = ' ';
        }
    }

    std::istringstream stream(line);
    std::vector<std::string> tokens;
    for (std::string token; stream >> token;) {
        tokens.push_back(token);
    }
    return tokens;
}

int parseRegister(const std::string& token) {
    if (token.size() < 2 || (token[0] != 'x' && token[0] != 'X')) {
        throw std::runtime_error("expected register like x3, got '" + token + "'");
    }
    const int reg = std::stoi(token.substr(1));
    if (reg < 0 || reg >= 32) {
        throw std::runtime_error("register out of range: " + token);
    }
    return reg;
}

std::int32_t parseImmediate(const std::string& token) {
    std::size_t consumed = 0;
    const long long value = std::stoll(token, &consumed, 0);
    if (consumed != token.size()) {
        throw std::runtime_error("invalid immediate: " + token);
    }
    return static_cast<std::int32_t>(value);
}

int parseTarget(const std::string& token,
                std::uint32_t pc,
                const std::unordered_map<std::string, int>& labels) {
    const auto found = labels.find(token);
    if (found != labels.end()) {
        return found->second;
    }

    const auto byteOffset = parseImmediate(token);
    return static_cast<int>(pc) + byteOffset / 4;
}

void requireCount(const std::vector<std::string>& tokens, std::size_t count, const std::string& opcode) {
    if (tokens.size() != count) {
        throw std::runtime_error(opcode + " expects " + std::to_string(count - 1) +
                                 " operands, got " + std::to_string(tokens.size() - 1));
    }
}

Opcode parseOpcode(const std::string& token) {
    const auto op = upper(token);
    if (op == "ADD") return Opcode::ADD;
    if (op == "SUB") return Opcode::SUB;
    if (op == "AND") return Opcode::AND;
    if (op == "OR") return Opcode::OR;
    if (op == "XOR") return Opcode::XOR;
    if (op == "SLL") return Opcode::SLL;
    if (op == "SRL") return Opcode::SRL;
    if (op == "SRA") return Opcode::SRA;
    if (op == "MUL") return Opcode::MUL;
    if (op == "MULH") return Opcode::MULH;
    if (op == "DIV") return Opcode::DIV;
    if (op == "REM") return Opcode::REM;
    if (op == "ADDI") return Opcode::ADDI;
    if (op == "ANDI") return Opcode::ANDI;
    if (op == "ORI") return Opcode::ORI;
    if (op == "XORI") return Opcode::XORI;
    if (op == "LW") return Opcode::LW;
    if (op == "SW") return Opcode::SW;
    if (op == "BEQ") return Opcode::BEQ;
    if (op == "BNE") return Opcode::BNE;
    if (op == "BLT") return Opcode::BLT;
    if (op == "BGE") return Opcode::BGE;
    if (op == "JAL") return Opcode::JAL;
    if (op == "JALR") return Opcode::JALR;
    if (op == "LUI") return Opcode::LUI;
    if (op == "AUIPC") return Opcode::AUIPC;
    if (op == "NOP") return Opcode::NOP;
    throw std::runtime_error("unknown opcode: " + token);
}

Instruction parseInstructionLine(const std::string& line,
                                 std::uint32_t pc,
                                 const std::unordered_map<std::string, int>& labels) {
    const auto tokens = tokenize(line);
    if (tokens.empty()) {
        throw std::runtime_error("internal parser error: empty instruction");
    }

    Instruction instruction;
    instruction.op = parseOpcode(tokens[0]);
    instruction.pc = pc;
    instruction.text = trim(line);

    switch (instruction.op) {
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::AND:
    case Opcode::OR:
    case Opcode::XOR:
    case Opcode::SLL:
    case Opcode::SRL:
    case Opcode::SRA:
    case Opcode::MUL:
    case Opcode::MULH:
    case Opcode::DIV:
    case Opcode::REM:
        requireCount(tokens, 4, tokens[0]);
        instruction.rd = parseRegister(tokens[1]);
        instruction.rs1 = parseRegister(tokens[2]);
        instruction.rs2 = parseRegister(tokens[3]);
        break;
    case Opcode::ADDI:
    case Opcode::ANDI:
    case Opcode::ORI:
    case Opcode::XORI:
        requireCount(tokens, 4, tokens[0]);
        instruction.rd = parseRegister(tokens[1]);
        instruction.rs1 = parseRegister(tokens[2]);
        instruction.imm = parseImmediate(tokens[3]);
        break;
    case Opcode::LW:
        requireCount(tokens, 4, tokens[0]);
        instruction.rd = parseRegister(tokens[1]);
        instruction.imm = parseImmediate(tokens[2]);
        instruction.rs1 = parseRegister(tokens[3]);
        break;
    case Opcode::SW:
        requireCount(tokens, 4, tokens[0]);
        instruction.rs2 = parseRegister(tokens[1]);
        instruction.imm = parseImmediate(tokens[2]);
        instruction.rs1 = parseRegister(tokens[3]);
        break;
    case Opcode::BEQ:
    case Opcode::BNE:
    case Opcode::BLT:
    case Opcode::BGE:
        requireCount(tokens, 4, tokens[0]);
        instruction.rs1 = parseRegister(tokens[1]);
        instruction.rs2 = parseRegister(tokens[2]);
        instruction.target = parseTarget(tokens[3], pc, labels);
        break;
    case Opcode::JAL:
        if (tokens.size() == 2) {
            instruction.rd = 1;
            instruction.target = parseTarget(tokens[1], pc, labels);
        } else {
            requireCount(tokens, 3, tokens[0]);
            instruction.rd = parseRegister(tokens[1]);
            instruction.target = parseTarget(tokens[2], pc, labels);
        }
        break;
    case Opcode::JALR:
        requireCount(tokens, 4, tokens[0]);
        instruction.rd = parseRegister(tokens[1]);
        instruction.imm = parseImmediate(tokens[2]);
        instruction.rs1 = parseRegister(tokens[3]);
        break;
    case Opcode::LUI:
    case Opcode::AUIPC:
        requireCount(tokens, 3, tokens[0]);
        instruction.rd = parseRegister(tokens[1]);
        instruction.imm = parseImmediate(tokens[2]);
        break;
    case Opcode::NOP:
        requireCount(tokens, 1, tokens[0]);
        break;
    }

    return instruction;
}

} // namespace

bool Instruction::usesRs1() const {
    switch (op) {
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::AND:
    case Opcode::OR:
    case Opcode::XOR:
    case Opcode::SLL:
    case Opcode::SRL:
    case Opcode::SRA:
    case Opcode::MUL:
    case Opcode::MULH:
    case Opcode::DIV:
    case Opcode::REM:
    case Opcode::ADDI:
    case Opcode::ANDI:
    case Opcode::ORI:
    case Opcode::XORI:
    case Opcode::LW:
    case Opcode::SW:
    case Opcode::BEQ:
    case Opcode::BNE:
    case Opcode::BLT:
    case Opcode::BGE:
    case Opcode::JALR:
        return true;
    case Opcode::JAL:
    case Opcode::LUI:
    case Opcode::AUIPC:
    case Opcode::NOP:
        return false;
    }
    return false;
}

bool Instruction::usesRs2() const {
    switch (op) {
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::AND:
    case Opcode::OR:
    case Opcode::XOR:
    case Opcode::SLL:
    case Opcode::SRL:
    case Opcode::SRA:
    case Opcode::MUL:
    case Opcode::MULH:
    case Opcode::DIV:
    case Opcode::REM:
    case Opcode::SW:
    case Opcode::BEQ:
    case Opcode::BNE:
    case Opcode::BLT:
    case Opcode::BGE:
        return true;
    default:
        return false;
    }
}

bool Instruction::writesRegister() const {
    switch (op) {
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::AND:
    case Opcode::OR:
    case Opcode::XOR:
    case Opcode::SLL:
    case Opcode::SRL:
    case Opcode::SRA:
    case Opcode::MUL:
    case Opcode::MULH:
    case Opcode::DIV:
    case Opcode::REM:
    case Opcode::ADDI:
    case Opcode::ANDI:
    case Opcode::ORI:
    case Opcode::XORI:
    case Opcode::LW:
    case Opcode::JAL:
    case Opcode::JALR:
    case Opcode::LUI:
    case Opcode::AUIPC:
        return rd != 0 && rd >= 0;
    case Opcode::SW:
    case Opcode::BEQ:
    case Opcode::BNE:
    case Opcode::BLT:
    case Opcode::BGE:
    case Opcode::NOP:
        return false;
    }
    return false;
}

bool Instruction::isLoad() const {
    return op == Opcode::LW;
}

bool Instruction::isStore() const {
    return op == Opcode::SW;
}

bool Instruction::isMemory() const {
    return isLoad() || isStore();
}

bool Instruction::isMultiplyDivide() const {
    return op == Opcode::MUL || op == Opcode::MULH || op == Opcode::DIV || op == Opcode::REM;
}

bool Instruction::isBranch() const {
    return op == Opcode::BEQ || op == Opcode::BNE || op == Opcode::BLT || op == Opcode::BGE;
}

bool Instruction::isJump() const {
    return op == Opcode::JAL || op == Opcode::JALR;
}

bool Instruction::isControl() const {
    return isBranch() || isJump();
}

std::string opcodeName(Opcode op) {
    switch (op) {
    case Opcode::ADD: return "ADD";
    case Opcode::SUB: return "SUB";
    case Opcode::AND: return "AND";
    case Opcode::OR: return "OR";
    case Opcode::XOR: return "XOR";
    case Opcode::SLL: return "SLL";
    case Opcode::SRL: return "SRL";
    case Opcode::SRA: return "SRA";
    case Opcode::MUL: return "MUL";
    case Opcode::MULH: return "MULH";
    case Opcode::DIV: return "DIV";
    case Opcode::REM: return "REM";
    case Opcode::ADDI: return "ADDI";
    case Opcode::ANDI: return "ANDI";
    case Opcode::ORI: return "ORI";
    case Opcode::XORI: return "XORI";
    case Opcode::LW: return "LW";
    case Opcode::SW: return "SW";
    case Opcode::BEQ: return "BEQ";
    case Opcode::BNE: return "BNE";
    case Opcode::BLT: return "BLT";
    case Opcode::BGE: return "BGE";
    case Opcode::JAL: return "JAL";
    case Opcode::JALR: return "JALR";
    case Opcode::LUI: return "LUI";
    case Opcode::AUIPC: return "AUIPC";
    case Opcode::NOP: return "NOP";
    }
    return "UNKNOWN";
}

std::vector<Instruction> parseProgram(std::istream& input) {
    std::unordered_map<std::string, int> labels;
    std::vector<std::string> instructionLines;

    std::string rawLine;
    int pc = 0;
    while (std::getline(input, rawLine)) {
        auto line = trim(stripComment(rawLine));
        while (!line.empty()) {
            const auto colon = line.find(':');
            if (colon == std::string::npos) {
                break;
            }
            const auto label = trim(line.substr(0, colon));
            if (label.empty()) {
                throw std::runtime_error("empty label");
            }
            labels[label] = pc;
            line = trim(line.substr(colon + 1));
        }

        if (!line.empty()) {
            instructionLines.push_back(line);
            ++pc;
        }
    }

    std::vector<Instruction> program;
    program.reserve(instructionLines.size());
    for (std::uint32_t i = 0; i < instructionLines.size(); ++i) {
        program.push_back(parseInstructionLine(instructionLines[i], i, labels));
    }
    return program;
}

std::vector<Instruction> parseProgramText(const std::string& text) {
    std::istringstream input(text);
    return parseProgram(input);
}

std::vector<Instruction> parseProgramFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("could not open program file: " + path);
    }
    return parseProgram(input);
}

std::string formatInstruction(const Instruction& instruction) {
    if (!instruction.text.empty()) {
        return instruction.text;
    }
    return opcodeName(instruction.op);
}

} // namespace ooo
