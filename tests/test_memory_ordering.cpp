#include "isa.hpp"
#include "reference_cpu.hpp"

#include <cassert>

int main() {
    const auto program = ooo::parseProgramText(R"(
        ADDI x1, x0, 99
        SW   x1, 96(x0)
        LW   x2, 96(x0)
        ADDI x3, x2, 1
        SW   x3, 100(x0)
        LW   x4, 100(x0)
    )");

    const auto result = ooo::runDifferentialTest(program, "memory_ordering");
    assert(result.passed);
}
