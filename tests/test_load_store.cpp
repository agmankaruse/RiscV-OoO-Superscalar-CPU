#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CPU cpu;
    cpu.loadProgramText(R"(
        ADDI x1, x0, 42
        SW   x1, 0(x0)
        LW   x2, 0(x0)
        ADDI x3, x2, 1
    )");
    cpu.run();

    assert(cpu.memory().readWord(0) == 42);
    assert(cpu.readArchitecturalRegister(2) == 42);
    assert(cpu.readArchitecturalRegister(3) == 43);
}
