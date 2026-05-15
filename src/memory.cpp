#include "memory.hpp"

namespace ooo {

std::uint8_t Memory::readByte(std::uint32_t address) const {
    const auto found = bytes_.find(address);
    if (found == bytes_.end()) {
        return 0;
    }
    return found->second;
}

void Memory::writeByte(std::uint32_t address, std::uint8_t value) {
    bytes_[address] = value;
}

std::uint32_t Memory::readWord(std::uint32_t address) const {
    std::uint32_t value = 0;
    value |= static_cast<std::uint32_t>(readByte(address));
    value |= static_cast<std::uint32_t>(readByte(address + 1)) << 8;
    value |= static_cast<std::uint32_t>(readByte(address + 2)) << 16;
    value |= static_cast<std::uint32_t>(readByte(address + 3)) << 24;
    return value;
}

void Memory::writeWord(std::uint32_t address, std::uint32_t value) {
    writeByte(address, static_cast<std::uint8_t>(value & 0xffu));
    writeByte(address + 1, static_cast<std::uint8_t>((value >> 8) & 0xffu));
    writeByte(address + 2, static_cast<std::uint8_t>((value >> 16) & 0xffu));
    writeByte(address + 3, static_cast<std::uint8_t>((value >> 24) & 0xffu));
}

void Memory::clear() {
    bytes_.clear();
}

} // namespace ooo
