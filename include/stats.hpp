#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace ooo {

struct Stats {
    std::uint64_t cycles = 0;
    std::uint64_t retiredInstructions = 0;
    std::uint64_t branchPredictions = 0;
    std::uint64_t correctBranchPredictions = 0;
    std::uint64_t branchMispredicts = 0;
    std::uint64_t mispredictionPenaltyCycles = 0;

    std::uint64_t instructionCacheHits = 0;
    std::uint64_t instructionCacheMisses = 0;
    std::uint64_t dataCacheHits = 0;
    std::uint64_t dataCacheMisses = 0;
    std::uint64_t fetchMissStalls = 0;
    std::uint64_t loadMissStalls = 0;

    std::uint64_t frontendStallCycles = 0;
    std::uint64_t backendStallCycles = 0;
    std::uint64_t branchMispredictStallCycles = 0;
    std::uint64_t cacheMissStallCycles = 0;
    std::uint64_t robFullStallCycles = 0;
    std::uint64_t iqFullStallCycles = 0;
    std::uint64_t lsqFullStallCycles = 0;
    std::uint64_t physicalRegisterStallCycles = 0;

    std::size_t robOccupancy = 0;
    std::size_t issueQueueOccupancy = 0;
    std::size_t loadStoreQueueOccupancy = 0;

    std::size_t maxRobOccupancy = 0;
    std::size_t maxIssueQueueOccupancy = 0;
    std::size_t maxLoadStoreQueueOccupancy = 0;

    std::uint64_t occupancySamples = 0;
    std::uint64_t totalRobOccupancy = 0;
    std::uint64_t totalIssueQueueOccupancy = 0;
    std::uint64_t totalLoadStoreQueueOccupancy = 0;

    void observeOccupancy(std::size_t rob, std::size_t iq, std::size_t lsq);
    double ipc() const;
    double cpi() const;
    double branchPredictionAccuracy() const;
    double instructionCacheHitRate() const;
    double dataCacheHitRate() const;
    double averageRobOccupancy() const;
    double averageIssueQueueOccupancy() const;
    double averageLoadStoreQueueOccupancy() const;
    std::uint64_t totalStallCycles() const;
    std::string cpiBreakdown() const;
    std::string summary() const;
};

} // namespace ooo
