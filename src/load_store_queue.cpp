#include "load_store_queue.hpp"

#include <algorithm>
#include <stdexcept>

namespace ooo {

LoadStoreQueue::LoadStoreQueue(std::size_t capacity) : capacity_(capacity) {}

bool LoadStoreQueue::canAllocate() const {
    return entries_.size() < capacity_;
}

void LoadStoreQueue::addLoad(std::uint64_t robId) {
    if (!canAllocate()) {
        throw std::runtime_error("LSQ is full");
    }
    entries_.push_back(LoadStoreEntry{robId, true});
}

void LoadStoreQueue::addStore(std::uint64_t robId) {
    if (!canAllocate()) {
        throw std::runtime_error("LSQ is full");
    }
    entries_.push_back(LoadStoreEntry{robId, false});
}

void LoadStoreQueue::markLoadComplete(std::uint64_t robId, std::uint32_t address, std::uint32_t value) {
    auto* entry = find(robId);
    if (!entry) {
        throw std::runtime_error("load missing from LSQ");
    }
    entry->addressReady = true;
    entry->valueReady = true;
    entry->address = address;
    entry->value = value;
}

void LoadStoreQueue::markStoreReady(std::uint64_t robId, std::uint32_t address, std::uint32_t value) {
    auto* entry = find(robId);
    if (!entry) {
        throw std::runtime_error("store missing from LSQ");
    }
    entry->addressReady = true;
    entry->valueReady = true;
    entry->address = address;
    entry->value = value;
}

LoadIssueInfo LoadStoreQueue::canLoadIssue(std::uint64_t robId, std::uint32_t address) const {
    LoadIssueInfo info;
    info.canIssue = true;

    for (const auto& entry : entries_) {
        if (entry.robId >= robId) {
            break;
        }
        if (entry.isLoad) {
            continue;
        }
        if (!entry.addressReady) {
            info.canIssue = false;
            return info;
        }
        if (entry.address == address) {
            if (!entry.valueReady) {
                info.canIssue = false;
                return info;
            }
            info.hasForwardedValue = true;
            info.forwardedValue = entry.value;
        }
    }

    return info;
}

void LoadStoreQueue::retire(std::uint64_t robId) {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [robId](const LoadStoreEntry& entry) {
        return entry.robId == robId;
    }), entries_.end());
}

void LoadStoreQueue::flushYoungerThan(std::uint64_t robId) {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [robId](const LoadStoreEntry& entry) {
        return entry.robId > robId;
    }), entries_.end());
}

void LoadStoreQueue::clear() {
    entries_.clear();
}

std::size_t LoadStoreQueue::size() const {
    return entries_.size();
}

std::size_t LoadStoreQueue::capacity() const {
    return capacity_;
}

bool LoadStoreQueue::empty() const {
    return entries_.empty();
}

const std::vector<LoadStoreEntry>& LoadStoreQueue::entries() const {
    return entries_;
}

LoadStoreEntry* LoadStoreQueue::find(std::uint64_t robId) {
    for (auto& entry : entries_) {
        if (entry.robId == robId) {
            return &entry;
        }
    }
    return nullptr;
}

const LoadStoreEntry* LoadStoreQueue::find(std::uint64_t robId) const {
    for (const auto& entry : entries_) {
        if (entry.robId == robId) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace ooo
