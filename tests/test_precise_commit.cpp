#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 2
        ADDI x2, x0, 3
        MUL  x3, x1, x2
        ADD  x4, x1, x2
        ADD  x5, x3, x4
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(3) == 6);
    assert(cpu.readArchitecturalRegister(4) == 5);
    assert(cpu.readArchitecturalRegister(5) == 11);
    assert(cpu.stats().retiredInstructions == 5);
}
