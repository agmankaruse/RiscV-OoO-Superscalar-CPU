#pragma once

#include <cstdint>
#include <vector>

namespace ooo {

// The physical register file stores speculative and committed values. Readiness
// bits are what let the issue queue wake up independent work out of order.
class PhysicalRegisterFile {
public:
    explicit PhysicalRegisterFile(int registerCount = 64);

    void reset();

    std::uint32_t read(int physicalRegister) const;
    void write(int physicalRegister, std::uint32_t value);

    bool isReady(int physicalRegister) const;
    void setReady(int physicalRegister, bool ready);

    int size() const;
    const std::vector<std::uint32_t>& values() const;
    std::vector<bool> readiness() const;

private:
    std::vector<std::uint32_t> values_;
    std::vector<bool> ready_;
};

} // namespace ooo
