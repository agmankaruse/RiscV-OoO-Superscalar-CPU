#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace ooo {
namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string normalize(std::string text) {
    text = trim(text);
    if (!text.empty() && text.front() == '"') {
        text.erase(text.begin());
    }
    if (!text.empty() && text.back() == ',') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '"') {
        text.pop_back();
    }
    std::replace(text.begin(), text.end(), '-', '_');
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return trim(text);
}

std::string stripComment(const std::string& line) {
    const auto hash = line.find('#');
    const auto slash = line.find("//");
    const auto cut = std::min(hash == std::string::npos ? line.size() : hash,
                              slash == std::string::npos ? line.size() : slash);
    return line.substr(0, cut);
}

int getInt(const std::unordered_map<std::string, std::string>& values,
           const std::string& key,
           int current) {
    const auto found = values.find(key);
    if (found == values.end()) {
        return current;
    }
    return std::stoi(found->second);
}

std::size_t getSize(const std::unordered_map<std::string, std::string>& values,
                    const std::string& key,
                    std::size_t current) {
    const auto found = values.find(key);
    if (found == values.end()) {
        return current;
    }
    return static_cast<std::size_t>(std::stoull(found->second));
}

} // namespace

std::string toString(BranchPredictorType type) {
    switch (type) {
    case BranchPredictorType::StaticNotTaken: return "static_not_taken";
    case BranchPredictorType::StaticTaken: return "static_taken";
    case BranchPredictorType::OneBit: return "one_bit";
    case BranchPredictorType::TwoBit: return "two_bit";
    case BranchPredictorType::Btb: return "btb";
    case BranchPredictorType::ReturnStack: return "return_stack";
    }
    return "static_not_taken";
}

BranchPredictorType branchPredictorTypeFromString(const std::string& text) {
    const auto value = normalize(text);
    if (value == "static_not_taken" || value == "not_taken") {
        return BranchPredictorType::StaticNotTaken;
    }
    if (value == "static_taken" || value == "taken") {
        return BranchPredictorType::StaticTaken;
    }
    if (value == "one_bit" || value == "1_bit" || value == "1bit") {
        return BranchPredictorType::OneBit;
    }
    if (value == "two_bit" || value == "2_bit" || value == "2bit") {
        return BranchPredictorType::TwoBit;
    }
    if (value == "btb") {
        return BranchPredictorType::Btb;
    }
    if (value == "return_stack" || value == "ras") {
        return BranchPredictorType::ReturnStack;
    }
    throw std::runtime_error("unknown branch predictor type: " + text);
}

CpuConfig loadCpuConfigFromFile(const std::string& path, CpuConfig config) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("could not open config file: " + path);
    }

    std::unordered_map<std::string, std::string> values;
    for (std::string rawLine; std::getline(input, rawLine);) {
        auto line = trim(stripComment(rawLine));
        if (line.empty() || line == "{" || line == "}") {
            continue;
        }
        if (!line.empty() && line.front() == '{') {
            line.erase(line.begin());
        }
        if (!line.empty() && line.back() == '}') {
            line.pop_back();
        }

        auto separator = line.find('=');
        if (separator == std::string::npos) {
            separator = line.find(':');
        }
        if (separator == std::string::npos) {
            continue;
        }

        auto key = normalize(line.substr(0, separator));
        auto value = normalize(line.substr(separator + 1));
        if (!key.empty() && !value.empty()) {
            values[key] = value;
        }
    }

    config.fetchWidth = getInt(values, "fetch_width", config.fetchWidth);
    config.decodeWidth = getInt(values, "decode_width", config.decodeWidth);
    config.renameWidth = getInt(values, "rename_width", config.renameWidth);
    config.dispatchWidth = getInt(values, "dispatch_width", config.dispatchWidth);
    config.issueWidth = getInt(values, "issue_width", config.issueWidth);
    config.commitWidth = getInt(values, "commit_width", config.commitWidth);
    config.robEntries = getSize(values, "rob_size", config.robEntries);
    config.robEntries = getSize(values, "rob_entries", config.robEntries);
    config.issueQueueEntries = getSize(values, "issue_queue_size", config.issueQueueEntries);
    config.physicalRegisters = getInt(values, "physical_register_count", config.physicalRegisters);
    config.loadStoreQueueEntries = getSize(values, "load_store_queue_size", config.loadStoreQueueEntries);
    config.loadStoreQueueEntries = getSize(values, "lsq_size", config.loadStoreQueueEntries);

    const auto predictor = values.find("branch_predictor");
    if (predictor != values.end()) {
        config.branchPredictorType = branchPredictorTypeFromString(predictor->second);
    }
    const auto predictorType = values.find("branch_predictor_type");
    if (predictorType != values.end()) {
        config.branchPredictorType = branchPredictorTypeFromString(predictorType->second);
    }

    config.mispredictPenaltyCycles = getInt(values, "mispredict_penalty_cycles", config.mispredictPenaltyCycles);
    config.l1InstructionCacheSize = getSize(values, "l1_instruction_cache_size", config.l1InstructionCacheSize);
    config.l1DataCacheSize = getSize(values, "l1_data_cache_size", config.l1DataCacheSize);
    config.cacheLineSize = getSize(values, "cache_line_size", config.cacheLineSize);
    config.l1CacheHitLatency = getInt(values, "l1_cache_hit_latency", config.l1CacheHitLatency);
    config.l1CacheMissLatency = getInt(values, "l1_cache_miss_latency", config.l1CacheMissLatency);
    config.aluLatency = getInt(values, "alu_latency", config.aluLatency);
    config.branchLatency = getInt(values, "branch_latency", config.branchLatency);
    config.mulLatency = getInt(values, "mul_latency", config.mulLatency);
    config.divLatency = getInt(values, "div_latency", config.divLatency);

    return config;
}

} // namespace ooo
