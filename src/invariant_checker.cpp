#include "invariant_checker.hpp"

#include "cpu.hpp"
#include "isa.hpp"

#include <set>
#include <sstream>

namespace ooo {
namespace {

void addViolation(InvariantReport& report, const std::string& rule, const std::string& detail) {
    report.violations.push_back({rule, detail});
}

bool validPhysicalRegister(int registerCount, int physicalRegister) {
    return physicalRegister >= 0 && physicalRegister < registerCount;
}

} // namespace

bool InvariantReport::passed() const {
    return violations.empty();
}

std::string InvariantReport::summary() const {
    std::ostringstream out;
    out << (passed() ? "PASS" : "FAIL") << " invariants";
    if (!passed()) {
        out << " violations=" << violations.size();
    }
    out << '\n';
    for (const auto& violation : violations) {
        out << violation.rule << ": " << violation.detail << '\n';
    }
    return out.str();
}

InvariantReport InvariantChecker::check(const CPU& cpu) {
    InvariantReport report;
    const auto physicalRegisterCount = cpu.registerFile_.size();

    if (cpu.readArchitecturalRegister(0) != 0 || cpu.renameTable_.currentMapping(0) != 0 ||
        cpu.renameTable_.committedMapping(0) != 0 || !cpu.registerFile_.isReady(0)) {
        addViolation(report, "x0_zero", "x0 must stay mapped to p0, ready, and architecturally zero");
    }

    std::set<int> mappedPhysicalRegisters;
    for (int arch = 0; arch < RenameTable::kArchitecturalRegisters; ++arch) {
        const auto current = cpu.renameTable_.currentMapping(arch);
        const auto committed = cpu.renameTable_.committedMapping(arch);
        if (!validPhysicalRegister(physicalRegisterCount, current)) {
            addViolation(report, "rename_current_range",
                         "x" + std::to_string(arch) + " maps to invalid p" + std::to_string(current));
        }
        if (!validPhysicalRegister(physicalRegisterCount, committed)) {
            addViolation(report, "rename_committed_range",
                         "x" + std::to_string(arch) + " commits to invalid p" + std::to_string(committed));
        }
        mappedPhysicalRegisters.insert(current);
        mappedPhysicalRegisters.insert(committed);
    }

    std::set<int> freePhysicalRegisters;
    for (const auto physical : cpu.renameTable_.freeListSnapshot()) {
        if (!validPhysicalRegister(physicalRegisterCount, physical) || physical == 0) {
            addViolation(report, "free_list_range", "invalid free physical register p" + std::to_string(physical));
        }
        if (!freePhysicalRegisters.insert(physical).second) {
            addViolation(report, "free_list_unique", "duplicate free physical register p" + std::to_string(physical));
        }
        if (mappedPhysicalRegisters.count(physical) != 0) {
            addViolation(report, "free_list_not_mapped",
                         "free list contains mapped physical register p" + std::to_string(physical));
        }
    }

    std::uint64_t previousRobId = 0;
    std::set<std::uint64_t> liveRobIds;
    for (const auto& entry : cpu.reorderBuffer_.entries()) {
        if (entry.id == 0) {
            addViolation(report, "rob_entry_valid", "occupied ROB entry has id 0");
        }
        if (entry.id <= previousRobId) {
            addViolation(report, "rob_program_order", "ROB ids are not strictly increasing");
        }
        previousRobId = entry.id;
        liveRobIds.insert(entry.id);

        if (entry.writesRegister) {
            if (!validPhysicalRegister(physicalRegisterCount, entry.physicalDestination)) {
                addViolation(report, "rob_destination_range",
                             "ROB" + std::to_string(entry.id) + " has invalid destination p" +
                                 std::to_string(entry.physicalDestination));
            }
            if (entry.ready && !cpu.registerFile_.isReady(entry.physicalDestination)) {
                addViolation(report, "ready_physical_register",
                             "ROB" + std::to_string(entry.id) + " is ready but p" +
                                 std::to_string(entry.physicalDestination) + " is not");
            }
        }
    }

    if (cpu.reorderBuffer_.size() > cpu.reorderBuffer_.capacity()) {
        addViolation(report, "rob_capacity", "ROB size exceeds capacity");
    }
    if (cpu.issueQueue_.size() > cpu.issueQueue_.capacity()) {
        addViolation(report, "iq_capacity", "issue queue size exceeds capacity");
    }
    if (cpu.loadStoreQueue_.size() > cpu.loadStoreQueue_.capacity()) {
        addViolation(report, "lsq_capacity", "LSQ size exceeds capacity");
    }

    for (const auto& entry : cpu.issueQueue_.entries()) {
        if (liveRobIds.count(entry.robId) == 0) {
            addViolation(report, "iq_rob_reference",
                         "issue entry references missing ROB" + std::to_string(entry.robId));
        }
        if (entry.instruction.usesRs1() && !validPhysicalRegister(physicalRegisterCount, entry.src1Physical)) {
            addViolation(report, "iq_src1_range", "issue entry has invalid src1 p" +
                         std::to_string(entry.src1Physical));
        }
        if (entry.instruction.usesRs2() && !validPhysicalRegister(physicalRegisterCount, entry.src2Physical)) {
            addViolation(report, "iq_src2_range", "issue entry has invalid src2 p" +
                         std::to_string(entry.src2Physical));
        }
    }

    for (const auto& entry : cpu.loadStoreQueue_.entries()) {
        if (liveRobIds.count(entry.robId) == 0) {
            addViolation(report, "lsq_rob_reference",
                         "LSQ entry references missing ROB" + std::to_string(entry.robId));
        }
    }

    return report;
}

} // namespace ooo
