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

std::string Stats::summary() const {
    std::ostringstream out;
    out << "cycles=" << cycles
        << " retired=" << retiredInstructions
        << " ipc=" << std::fixed << std::setprecision(2) << ipc()
        << " branch_mispredicts=" << branchMispredicts
        << " rob=" << robOccupancy << "/" << maxRobOccupancy
        << " iq=" << issueQueueOccupancy << "/" << maxIssueQueueOccupancy
        << " lsq=" << loadStoreQueueOccupancy << "/" << maxLoadStoreQueueOccupancy;
    return out.str();
}

} // namespace ooo
