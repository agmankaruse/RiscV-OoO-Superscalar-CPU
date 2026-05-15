#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 1
        ADDI x2, x0, 2
        ADD  x1, x1, x2
        ADD  x3, x1, x2
        ADDI x1, x0, 9
        ADD  x4, x3, x2
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(1) == 9);
    assert(cpu.readArchitecturalRegister(2) == 2);
    assert(cpu.readArchitecturalRegister(3) == 5);
    assert(cpu.readArchitecturalRegister(4) == 7);
}
