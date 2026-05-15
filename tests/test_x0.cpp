#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x0, x0, 123
        ADDI x1, x0, 5
        ADD  x0, x1, x1
        ADD  x2, x0, x1
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(0) == 0);
    assert(cpu.readArchitecturalRegister(1) == 5);
    assert(cpu.readArchitecturalRegister(2) == 5);
}
