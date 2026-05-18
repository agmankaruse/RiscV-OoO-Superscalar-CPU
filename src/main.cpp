#include "cpu.hpp"
#include "config.hpp"
#include "invariant_checker.hpp"
#include "reference_cpu.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage(const char* programName) {
    std::cerr << "usage: " << programName
              << " [--config <config-file>] [--trace] [--timeline] [--timeline-csv <path>]\n"
              << "       [--stats-csv <path>] [--diff] [--check-invariants] [--dump-on-fail]\n"
              << "       [--version] <program.txt>\n";
}

} // namespace

int main(int argc, char** argv) {
    bool trace = false;
    bool timeline = false;
    bool diff = false;
    bool checkInvariants = false;
    bool dumpOnFail = false;
    std::string configPath;
    std::string timelineCsvPath = "outputs/cpu_pipeline_timeline.csv";
    std::string statsCsvPath;
    std::string programPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--trace") {
            trace = true;
        } else if (arg == "--timeline") {
            timeline = true;
        } else if (arg == "--diff") {
            diff = true;
        } else if (arg == "--check-invariants") {
            checkInvariants = true;
        } else if (arg == "--dump-on-fail") {
            dumpOnFail = true;
        } else if (arg == "--version") {
            std::cout << "riscv_ooo_sim 0.1.0\n";
            return 0;
        } else if (arg == "--config") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            configPath = argv[i];
        } else if (arg == "--timeline-csv") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            timelineCsvPath = argv[i];
            timeline = true;
        } else if (arg == "--stats-csv") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            statsCsvPath = argv[i];
        } else if (programPath.empty()) {
            programPath = arg;
        } else {
            printUsage(argv[0]);
            return 2;
        }
    }

    if (programPath.empty()) {
        printUsage(argv[0]);
        return 2;
    }

    try {
        auto config = ooo::CpuConfig{};
        if (!configPath.empty()) {
            config = ooo::loadCpuConfigFromFile(configPath, config);
        }

        if (diff) {
            const auto result = ooo::runDifferentialTestFromFile(programPath, config);
            std::cout << ooo::formatDiffResult(result);
            if (!result.passed && dumpOnFail) {
                ooo::CPU dumpCpu(config);
                dumpCpu.loadProgramFromFile(programPath);
                dumpCpu.run();
                std::cout << dumpCpu.dumpPipelineState();
            }
            return result.passed ? 0 : 1;
        }

        ooo::CPU cpu(config);
        cpu.setTrace(trace);
        if (timeline) {
            cpu.enableTimelineCsv(timelineCsvPath);
        }
        if (!statsCsvPath.empty()) {
            cpu.enableStatsCsv(statsCsvPath);
        }
        cpu.loadProgramFromFile(programPath);
        if (checkInvariants) {
            while (!cpu.halted()) {
                if (cpu.stats().cycles >= 100000) {
                    throw std::runtime_error("simulation did not halt before max cycle limit");
                }
                cpu.tick();
                const auto report = ooo::InvariantChecker::check(cpu);
                if (!report.passed()) {
                    std::cout << report.summary();
                    if (dumpOnFail) {
                        std::cout << cpu.dumpPipelineState();
                    }
                    return 1;
                }
            }
            std::cout << ooo::InvariantChecker::check(cpu).summary();
        } else {
            cpu.run();
        }

        std::cout << cpu.stats().summary() << "\n";
        std::cout << cpu.stats().cpiBreakdown() << "\n";
        if (timeline) {
            std::cout << "timeline_csv=" << timelineCsvPath << "\n";
        }
        if (!statsCsvPath.empty()) {
            std::cout << "stats_csv=" << statsCsvPath << "\n";
        }
        std::cout << "architectural registers:";
        for (int reg = 0; reg < 32; ++reg) {
            const auto value = cpu.readArchitecturalRegister(reg);
            if (value != 0 || reg == 0) {
                std::cout << " x" << reg << "=" << value;
            }
        }
        std::cout << "\n";
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
