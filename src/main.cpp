#include "cpu.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

void printUsage(const char* programName) {
    std::cerr << "usage: " << programName << " [--trace] <program.txt>\n";
}

} // namespace

int main(int argc, char** argv) {
    bool trace = false;
    std::string programPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--trace") {
            trace = true;
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
        ooo::CPU cpu;
        cpu.setTrace(trace);
        cpu.loadProgramFromFile(programPath);
        cpu.run();

        std::cout << cpu.stats().summary() << "\n";
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
