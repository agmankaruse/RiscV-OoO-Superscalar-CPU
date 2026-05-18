#include "stats.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ooo {

void Stats::observeOccupancy(std::size_t rob, std::size_t iq, std::size_t lsq) {
    robOccupancy = rob;
    issueQueueOccupancy = iq;
    loadStoreQueueOccupancy = lsq;

    maxRobOccupancy = std::max(maxRobOccupancy, rob);
    maxIssueQueueOccupancy = std::max(maxIssueQueueOccupancy, iq);
    maxLoadStoreQueueOccupancy = std::max(maxLoadStoreQueueOccupancy, lsq);

    ++occupancySamples;
    totalRobOccupancy += rob;
    totalIssueQueueOccupancy += iq;
    totalLoadStoreQueueOccupancy += lsq;
}

double Stats::ipc() const {
    if (cycles == 0) {
        return 0.0;
    }
    return static_cast<double>(retiredInstructions) / static_cast<double>(cycles);
}

double Stats::cpi() const {
    if (retiredInstructions == 0) {
        return 0.0;
    }
    return static_cast<double>(cycles) / static_cast<double>(retiredInstructions);
}

double Stats::branchPredictionAccuracy() const {
    if (branchPredictions == 0) {
        return 0.0;
    }
    return static_cast<double>(correctBranchPredictions) / static_cast<double>(branchPredictions);
}

double Stats::instructionCacheHitRate() const {
    const auto accesses = instructionCacheHits + instructionCacheMisses;
    if (accesses == 0) {
        return 0.0;
    }
    return static_cast<double>(instructionCacheHits) / static_cast<double>(accesses);
}

double Stats::dataCacheHitRate() const {
    const auto accesses = dataCacheHits + dataCacheMisses;
    if (accesses == 0) {
        return 0.0;
    }
    return static_cast<double>(dataCacheHits) / static_cast<double>(accesses);
}

double Stats::averageRobOccupancy() const {
    if (occupancySamples == 0) {
        return 0.0;
    }
    return static_cast<double>(totalRobOccupancy) / static_cast<double>(occupancySamples);
}

double Stats::averageIssueQueueOccupancy() const {
    if (occupancySamples == 0) {
        return 0.0;
    }
    return static_cast<double>(totalIssueQueueOccupancy) / static_cast<double>(occupancySamples);
}

double Stats::averageLoadStoreQueueOccupancy() const {
    if (occupancySamples == 0) {
        return 0.0;
    }
    return static_cast<double>(totalLoadStoreQueueOccupancy) / static_cast<double>(occupancySamples);
}

std::uint64_t Stats::totalStallCycles() const {
    return frontendStallCycles + backendStallCycles + branchMispredictStallCycles + cacheMissStallCycles;
}

std::string Stats::cpiBreakdown() const {
    std::ostringstream out;
    out << "cpi=" << std::fixed << std::setprecision(2) << cpi()
        << " stalls_total=" << totalStallCycles()
        << " frontend=" << frontendStallCycles
        << " backend=" << backendStallCycles
        << " branch_mispredict=" << branchMispredictStallCycles
        << " cache_miss=" << cacheMissStallCycles
        << " rob_full=" << robFullStallCycles
        << " iq_full=" << iqFullStallCycles
        << " lsq_full=" << lsqFullStallCycles
        << " physical_register=" << physicalRegisterStallCycles;
    return out.str();
}

std::string Stats::summary() const {
    std::ostringstream out;
    out << "cycles=" << cycles
        << " retired=" << retiredInstructions
        << " ipc=" << std::fixed << std::setprecision(2) << ipc()
        << " cpi=" << std::fixed << std::setprecision(2) << cpi()
        << " branch_accuracy=" << std::fixed << std::setprecision(2) << (branchPredictionAccuracy() * 100.0) << "%"
        << " branch_mispredicts=" << branchMispredicts
        << " icache_hits=" << instructionCacheHits
        << " icache_misses=" << instructionCacheMisses
        << " icache_hit_rate=" << std::fixed << std::setprecision(2) << (instructionCacheHitRate() * 100.0) << "%"
        << " dcache_hits=" << dataCacheHits
        << " dcache_misses=" << dataCacheMisses
        << " dcache_hit_rate=" << std::fixed << std::setprecision(2) << (dataCacheHitRate() * 100.0) << "%"
        << " fetch_miss_stalls=" << fetchMissStalls
        << " load_miss_stalls=" << loadMissStalls
        << " frontend_stalls=" << frontendStallCycles
        << " backend_stalls=" << backendStallCycles
        << " branch_mispredict_stalls=" << branchMispredictStallCycles
        << " cache_miss_stalls=" << cacheMissStallCycles
        << " rob_full_stalls=" << robFullStallCycles
        << " iq_full_stalls=" << iqFullStallCycles
        << " lsq_full_stalls=" << lsqFullStallCycles
        << " physical_register_stalls=" << physicalRegisterStallCycles
        << " avg_rob=" << std::fixed << std::setprecision(2) << averageRobOccupancy()
        << " avg_iq=" << std::fixed << std::setprecision(2) << averageIssueQueueOccupancy()
        << " avg_lsq=" << std::fixed << std::setprecision(2) << averageLoadStoreQueueOccupancy()
        << " rob=" << robOccupancy << "/" << maxRobOccupancy
        << " iq=" << issueQueueOccupancy << "/" << maxIssueQueueOccupancy
        << " lsq=" << loadStoreQueueOccupancy << "/" << maxLoadStoreQueueOccupancy;
    return out.str();
}

} // namespace ooo
