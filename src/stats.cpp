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
}

double Stats::ipc() const {
    if (cycles == 0) {
        return 0.0;
    }
    return static_cast<double>(retiredInstructions) / static_cast<double>(cycles);
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

std::string Stats::summary() const {
    std::ostringstream out;
    out << "cycles=" << cycles
        << " retired=" << retiredInstructions
        << " ipc=" << std::fixed << std::setprecision(2) << ipc()
        << " branch_accuracy=" << std::fixed << std::setprecision(2) << (branchPredictionAccuracy() * 100.0) << "%"
        << " branch_mispredicts=" << branchMispredicts
        << " icache_hits=" << instructionCacheHits
        << " icache_misses=" << instructionCacheMisses
        << " dcache_hits=" << dataCacheHits
        << " dcache_misses=" << dataCacheMisses
        << " fetch_miss_stalls=" << fetchMissStalls
        << " load_miss_stalls=" << loadMissStalls
        << " rob=" << robOccupancy << "/" << maxRobOccupancy
        << " iq=" << issueQueueOccupancy << "/" << maxIssueQueueOccupancy
        << " lsq=" << loadStoreQueueOccupancy << "/" << maxLoadStoreQueueOccupancy;
    return out.str();
}

} // namespace ooo
