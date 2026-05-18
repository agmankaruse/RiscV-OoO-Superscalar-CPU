#include "isa.hpp"
#include "reference_cpu.hpp"

#include <cassert>

int main() {
    const auto program = ooo::parseProgramText(R"(
        ADDI x1, x0, 0
        ADDI x2, x0, 3
    loop:
        ADDI x1, x1, 1
        BNE  x1, x2, loop
        ADDI x3, x1, 7
    )");

    const auto result = ooo::runDifferentialTest(program, "branch_loop_flush");
    assert(result.passed);
}
