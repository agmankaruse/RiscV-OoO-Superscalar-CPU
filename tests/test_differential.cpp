#include "isa.hpp"
#include "reference_cpu.hpp"

#include <cassert>

int main() {
    const auto program = ooo::parseProgramText(R"(
        ADDI x1, x0, 10
        ADDI x2, x0, 20
        ADD  x3, x1, x2
        XOR  x4, x3, x1
        SW   x4, 16(x0)
        LW   x5, 16(x0)
    )");

    const auto result = ooo::runDifferentialTest(program, "inline_arithmetic_memory");
    assert(result.passed);
}
