#include "reference_cpu.hpp"

#include <cassert>

int main() {
    ooo::ReferenceCPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 7
        ADDI x2, x0, 5
        ADD  x3, x1, x2
        SW   x3, 0(x0)
        LW   x4, 0(x0)
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(0) == 0);
    assert(cpu.readArchitecturalRegister(1) == 7);
    assert(cpu.readArchitecturalRegister(2) == 5);
    assert(cpu.readArchitecturalRegister(3) == 12);
    assert(cpu.readArchitecturalRegister(4) == 12);
    assert(cpu.memory().readWord(0) == 12);
    assert(cpu.retiredInstructions() == 5);
}
