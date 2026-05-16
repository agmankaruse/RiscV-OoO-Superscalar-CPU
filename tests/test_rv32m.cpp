#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::CpuConfig config;
    config.l1CacheMissLatency = 1;
    config.mulLatency = 3;
    config.divLatency = 7;

    ooo::CPU cpu(config);
    cpu.loadProgramText(R"(
        ADDI x1, x0, 6
        ADDI x2, x0, 7
        MUL  x3, x1, x2
        DIV  x4, x3, x1
        REM  x5, x3, x2
        DIV  x6, x3, x0
    )");
    cpu.run();

    assert(cpu.readArchitecturalRegister(3) == 42);
    assert(cpu.readArchitecturalRegister(4) == 7);
    assert(cpu.readArchitecturalRegister(5) == 0);
    assert(cpu.readArchitecturalRegister(6) == 0xffffffffu);
}
