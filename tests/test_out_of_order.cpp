#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        LW   x1, 0(x0)
        ADDI x2, x0, 7
        ADDI x3, x0, 8
        ADD  x4, x2, x3
        ADD  x5, x1, x4
    )");
    cpu.memory().writeWord(0, 100);
    cpu.run();

    assert(cpu.readArchitecturalRegister(1) == 100);
    assert(cpu.readArchitecturalRegister(4) == 15);
    assert(cpu.readArchitecturalRegister(5) == 115);
    assert(cpu.stats().maxIssueQueueOccupancy > 0);
}
