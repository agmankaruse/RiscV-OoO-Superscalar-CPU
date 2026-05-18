#include "cpu.hpp"
#include "invariant_checker.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 1
        ADDI x2, x1, 2
        ADD  x3, x1, x2
        SW   x3, 32(x0)
        LW   x4, 32(x0)
    )");

    while (!cpu.halted()) {
        cpu.tick();
        const auto report = ooo::InvariantChecker::check(cpu);
        assert(report.passed());
    }
}
