#include "cache.hpp"
#include "cpu.hpp"

#include <cassert>

int main() {
    ooo::DirectMappedCache cache(64, 16, 1, 5);
    auto first = cache.access(0);
    auto second = cache.access(4);
    auto conflict = cache.access(64);

    assert(!first.hit);
    assert(first.latencyCycles == 5);
    assert(second.hit);
    assert(conflict.hit == false);
    assert(cache.hits() == 1);
    assert(cache.misses() == 2);

    ooo::CpuConfig config;
    config.l1InstructionCacheSize = 32;
    config.l1DataCacheSize = 32;
    config.cacheLineSize = 16;
    config.l1CacheMissLatency = 3;

    ooo::CPU cpu(config);
    cpu.loadProgramText(R"(
        LW   x1, 0(x0)
        LW   x2, 0(x0)
        ADD  x3, x1, x2
    )");
    cpu.memory().writeWord(0, 123);
    cpu.run();

    assert(cpu.readArchitecturalRegister(3) == 246);
    assert(cpu.stats().instructionCacheMisses > 0);
    assert(cpu.stats().dataCacheMisses > 0);
    assert(cpu.stats().dataCacheHits > 0);
}
