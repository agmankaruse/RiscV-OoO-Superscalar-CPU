#include "cpu.hpp"

#include <cassert>

namespace {

const char* loopProgram = R"(
    ADDI x1, x0, 5
    ADDI x2, x0, 0
loop:
    ADDI x2, x2, 1
    ADDI x1, x1, -1
    BNE  x1, x0, loop
    ADD  x3, x2, x0
)";

} // namespace

int main() {
    ooo::CpuConfig staticConfig;
    staticConfig.branchPredictorType = ooo::BranchPredictorType::StaticNotTaken;
    staticConfig.l1CacheMissLatency = 1;

    ooo::CPU staticCpu(staticConfig);
    staticCpu.loadProgramText(loopProgram);
    staticCpu.run();

    ooo::CpuConfig twoBitConfig = staticConfig;
    twoBitConfig.branchPredictorType = ooo::BranchPredictorType::TwoBit;

    ooo::CPU twoBitCpu(twoBitConfig);
    twoBitCpu.loadProgramText(loopProgram);
    twoBitCpu.run();

    assert(staticCpu.readArchitecturalRegister(3) == 5);
    assert(twoBitCpu.readArchitecturalRegister(3) == 5);
    assert(staticCpu.stats().branchMispredicts > twoBitCpu.stats().branchMispredicts);
    assert(twoBitCpu.stats().branchPredictionAccuracy() > 0.0);
}
