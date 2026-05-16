#include "cpu.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    ooo::CpuConfig config;
    config.l1CacheMissLatency = 1;

    const std::string path = "test_pipeline_timeline.csv";
    ooo::CPU cpu(config);
    cpu.enableTimelineCsv(path);
    cpu.loadProgramText(R"(
        ADDI x1, x0, 1
        ADDI x2, x1, 2
    )");
    cpu.run();

    std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const auto text = buffer.str();

    assert(text.find("FETCH") != std::string::npos);
    assert(text.find("RENAME") != std::string::npos);
    assert(text.find("COMMIT") != std::string::npos);
}
