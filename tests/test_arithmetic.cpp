#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 5
        ADDI x2, x0, 10
        ADD  x3, x1, x2
        SUB  x4, x3, x1
        XORI x5, x4, 3
        SLL  x6, x1, x0
        OR   x7, x5, x6
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(1) == 5);
    assert(cpu.readArchitecturalRegister(2) == 10);
    assert(cpu.readArchitecturalRegister(3) == 15);
    assert(cpu.readArchitecturalRegister(4) == 10);
    assert(cpu.readArchitecturalRegister(5) == 9);
    assert(cpu.readArchitecturalRegister(6) == 5);
    assert(cpu.readArchitecturalRegister(7) == 13);
    assert(cpu.stats().retiredInstructions == 7);
}
