#include "config.hpp"

#include <cassert>
#include <fstream>

int main() {
    const char* path = "test_cpu_config.json";
    {
        std::ofstream output(path);
        output << "{\n"
               << "  \"fetch_width\": 3,\n"
               << "  \"issue_width\": 5,\n"
               << "  \"rob_size\": 48,\n"
               << "  \"branch_predictor\": \"two_bit\",\n"
               << "  \"l1_data_cache_size\": 4096,\n"
               << "  \"mul_latency\": 4\n"
               << "}\n";
    }

    auto config = ooo::loadCpuConfigFromFile(path);
    assert(config.fetchWidth == 3);
    assert(config.issueWidth == 5);
    assert(config.robEntries == 48);
    assert(config.branchPredictorType == ooo::BranchPredictorType::TwoBit);
    assert(config.l1DataCacheSize == 4096);
    assert(config.mulLatency == 4);
}
