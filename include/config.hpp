#pragma once

#include <cstddef>
#include <string>

namespace ooo {

enum class BranchPredictorType {
    StaticNotTaken,
    StaticTaken,
    OneBit,
    TwoBit,
    Btb,
    ReturnStack
};

struct CpuConfig {
    int fetchWidth = 2;
    int decodeWidth = 2;
    int renameWidth = 2;
    int dispatchWidth = 2;
    int issueWidth = 4;
    int commitWidth = 2;

    std::size_t robEntries = 32;
    std::size_t issueQueueEntries = 16;
    std::size_t loadStoreQueueEntries = 16;
    int physicalRegisters = 64;

    BranchPredictorType branchPredictorType = BranchPredictorType::StaticNotTaken;
    int mispredictPenaltyCycles = 3;

    std::size_t l1InstructionCacheSize = 1024;
    std::size_t l1DataCacheSize = 1024;
    std::size_t cacheLineSize = 32;
    int l1CacheHitLatency = 1;
    int l1CacheMissLatency = 8;

    int aluLatency = 1;
    int branchLatency = 1;
    int mulLatency = 3;
    int divLatency = 12;
};

std::string toString(BranchPredictorType type);
BranchPredictorType branchPredictorTypeFromString(const std::string& text);
CpuConfig loadCpuConfigFromFile(const std::string& path, CpuConfig base = CpuConfig{});

} // namespace ooo
