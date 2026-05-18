#pragma once

#include <string>
#include <vector>

namespace ooo {

class CPU;

struct InvariantViolation {
    std::string rule;
    std::string detail;
};

struct InvariantReport {
    std::vector<InvariantViolation> violations;

    bool passed() const;
    std::string summary() const;
};

class InvariantChecker {
public:
    static InvariantReport check(const CPU& cpu);
};

} // namespace ooo
