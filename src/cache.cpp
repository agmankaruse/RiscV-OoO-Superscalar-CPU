#include "cache.hpp"

#include <algorithm>

namespace ooo {

DirectMappedCache::DirectMappedCache(std::size_t sizeBytes,
                                     std::size_t lineSizeBytes,
                                     int hitLatencyCycles,
                                     int missLatencyCycles) {
    configure(sizeBytes, lineSizeBytes, hitLatencyCycles, missLatencyCycles);
}

void DirectMappedCache::configure(std::size_t sizeBytes,
                                  std::size_t lineSizeBytes,
                                  int hitLatencyCycles,
                                  int missLatencyCycles) {
    sizeBytes_ = std::max<std::size_t>(lineSizeBytes, sizeBytes);
    lineSizeBytes_ = std::max<std::size_t>(1, lineSizeBytes);
    hitLatencyCycles_ = std::max(1, hitLatencyCycles);
    missLatencyCycles_ = std::max(hitLatencyCycles_, missLatencyCycles);

    const auto lineCount = std::max<std::size_t>(1, sizeBytes_ / lineSizeBytes_);
    lines_.assign(lineCount, Line{});
    hits_ = 0;
    misses_ = 0;
}

void DirectMappedCache::reset() {
    for (auto& line : lines_) {
        line = Line{};
    }
    hits_ = 0;
    misses_ = 0;
}

CacheAccess DirectMappedCache::access(std::uint32_t address) {
    const auto block = address / static_cast<std::uint32_t>(lineSizeBytes_);
    const auto index = static_cast<std::size_t>(block % lines_.size());
    const auto tag = block / static_cast<std::uint32_t>(lines_.size());

    auto& line = lines_[index];
    if (line.valid && line.tag == tag) {
        ++hits_;
        return CacheAccess{true, hitLatencyCycles_};
    }

    ++misses_;
    line.valid = true;
    line.tag = tag;
    return CacheAccess{false, missLatencyCycles_};
}

std::uint64_t DirectMappedCache::hits() const {
    return hits_;
}

std::uint64_t DirectMappedCache::misses() const {
    return misses_;
}

double DirectMappedCache::hitRate() const {
    const auto accesses = hits_ + misses_;
    if (accesses == 0) {
        return 0.0;
    }
    return static_cast<double>(hits_) / static_cast<double>(accesses);
}

} // namespace ooo
