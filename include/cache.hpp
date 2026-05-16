#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ooo {

struct CacheAccess {
    bool hit = false;
    int latencyCycles = 1;
};

// Small direct-mapped L1 cache model. It tracks tags and hit/miss latency; it
// deliberately leaves coherence and write policy simple for teaching clarity.
class DirectMappedCache {
public:
    DirectMappedCache(std::size_t sizeBytes = 1024,
                      std::size_t lineSizeBytes = 32,
                      int hitLatencyCycles = 1,
                      int missLatencyCycles = 8);

    void configure(std::size_t sizeBytes,
                   std::size_t lineSizeBytes,
                   int hitLatencyCycles,
                   int missLatencyCycles);
    void reset();

    CacheAccess access(std::uint32_t address);

    std::uint64_t hits() const;
    std::uint64_t misses() const;
    double hitRate() const;

private:
    struct Line {
        bool valid = false;
        std::uint32_t tag = 0;
    };

    std::size_t sizeBytes_ = 1024;
    std::size_t lineSizeBytes_ = 32;
    int hitLatencyCycles_ = 1;
    int missLatencyCycles_ = 8;
    std::vector<Line> lines_;
    std::uint64_t hits_ = 0;
    std::uint64_t misses_ = 0;
};

} // namespace ooo
