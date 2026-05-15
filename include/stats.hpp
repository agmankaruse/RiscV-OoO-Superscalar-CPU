#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace ooo {

struct Stats {
    std::uint64_t cycles = 0;
    std::uint64_t retiredInstructions = 0;
    std::uint64_t branchMispredicts = 0;

    std::size_t robOccupancy = 0;
    std::size_t issueQueueOccupancy = 0;
    std::size_t loadStoreQueueOccupancy = 0;

    std::size_t maxRobOccupancy = 0;
    std::size_t maxIssueQueueOccupancy = 0;
    std::size_t maxLoadStoreQueueOccupancy = 0;

    void observeOccupancy(std::size_t rob, std::size_t iq, std::size_t lsq);
    double ipc() const;
    std::string summary() const;
};

} // namespace ooo
