#include "cpu.hpp"
#include "config.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

void printUsage(const char* programName) {
    std::cerr << "usage: " << programName
              << " [--config <config-file>] [--trace] [--timeline] [--stats-csv <path>] <program.txt>\n";
}

} // namespace

int main(int argc, char** argv) {
    bool trace = false;
    bool timeline = false;
    std::string configPath;
    std::string statsCsvPath;
    std::string programPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--trace") {
            trace = true;
        } else if (arg == "--timeline") {
            timeline = true;
        } else if (arg == "--config") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            configPath = argv[i];
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

        ooo::CPU cpu(config);
        cpu.setTrace(trace);
        if (timeline) {
            cpu.enableTimelineCsv("outputs/pipeline_timeline.csv");
        }
        if (!statsCsvPath.empty()) {
            cpu.enableStatsCsv(statsCsvPath);
        }
        cpu.loadProgramFromFile(programPath);
        cpu.run();

        std::cout << cpu.stats().summary() << "\n";
        if (timeline) {
            std::cout << "timeline_csv=outputs/pipeline_timeline.csv\n";
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
