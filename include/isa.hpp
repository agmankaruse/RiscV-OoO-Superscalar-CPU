#pragma once

#include "instruction.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace ooo {

std::vector<Instruction> parseProgram(std::istream& input);
std::vector<Instruction> parseProgramText(const std::string& text);
std::vector<Instruction> parseProgramFile(const std::string& path);
std::string formatInstruction(const Instruction& instruction);

} // namespace ooo
