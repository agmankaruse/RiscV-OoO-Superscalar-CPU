#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 5
        ADDI x2, x0, 5
        BEQ  x1, x2, taken
        ADDI x3, x0, 99
    taken:
        ADDI x3, x0, 7
        BNE  x1, x2, skipped
        ADDI x4, x0, 11
    skipped:
        ADDI x5, x0, 12
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(3) == 7);
    assert(cpu.readArchitecturalRegister(4) == 11);
    assert(cpu.readArchitecturalRegister(5) == 12);
    assert(cpu.stats().branchMispredicts == 1);
}
