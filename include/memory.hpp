#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ooo {

// Sparse little-endian byte-addressable memory. The simulator only needs word
// accesses today, but bytes make it easy to extend toward RV32I loads/stores.
class Memory {
public:
    std::uint8_t readByte(std::uint32_t address) const;
    void writeByte(std::uint32_t address, std::uint8_t value);

    std::uint32_t readWord(std::uint32_t address) const;
    void writeWord(std::uint32_t address, std::uint32_t value);

    std::vector<std::pair<std::uint32_t, std::uint32_t>> words() const;

    void clear();

private:
    std::unordered_map<std::uint32_t, std::uint8_t> bytes_;
};

} // namespace ooo
