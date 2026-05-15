#include "reorder_buffer.hpp"

#include <algorithm>
#include <stdexcept>

namespace ooo {

ReorderBuffer::ReorderBuffer(std::size_t capacity) : capacity_(capacity) {}

void ReorderBuffer::clear() {
    entries_.clear();
}

bool ReorderBuffer::canAllocate() const {
    return entries_.size() < capacity_;
}

std::size_t ReorderBuffer::capacity() const {
    return capacity_;
}

std::size_t ReorderBuffer::size() const {
    return entries_.size();
}

bool ReorderBuffer::empty() const {
    return entries_.empty();
}

std::uint64_t ReorderBuffer::allocate(RobEntry entry) {
    if (!canAllocate()) {
        throw std::runtime_error("ROB is full");
    }
    entries_.push_back(entry);
    return entry.id;
}

RobEntry* ReorderBuffer::find(std::uint64_t id) {
    for (auto& entry : entries_) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

const RobEntry* ReorderBuffer::find(std::uint64_t id) const {
    for (const auto& entry : entries_) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

RobEntry& ReorderBuffer::head() {
    if (entries_.empty()) {
        throw std::runtime_error("ROB head requested while empty");
    }
    return entries_.front();
}

const RobEntry& ReorderBuffer::head() const {
    if (entries_.empty()) {
        throw std::runtime_error("ROB head requested while empty");
    }
    return entries_.front();
}

void ReorderBuffer::popHead() {
    if (entries_.empty()) {
        throw std::runtime_error("ROB pop requested while empty");
    }
    entries_.pop_front();
}

std::vector<RobEntry> ReorderBuffer::flushYoungerThan(std::uint64_t id) {
    std::vector<RobEntry> removed;
    auto firstYounger = std::find_if(entries_.begin(), entries_.end(), [id](const RobEntry& entry) {
        return entry.id > id;
    });
    while (firstYounger != entries_.end()) {
        removed.push_back(*firstYounger);
        firstYounger = entries_.erase(firstYounger);
    }
    return removed;
}

const std::deque<RobEntry>& ReorderBuffer::entries() const {
    return entries_;
}

} // namespace ooo
