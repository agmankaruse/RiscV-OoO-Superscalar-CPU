#include "issue_queue.hpp"

#include <algorithm>
#include <stdexcept>

namespace ooo {

IssueQueue::IssueQueue(std::size_t capacity) : capacity_(capacity) {}

bool IssueQueue::canAllocate() const {
    return entries_.size() < capacity_;
}

void IssueQueue::add(const IssueEntry& entry) {
    if (!canAllocate()) {
        throw std::runtime_error("issue queue is full");
    }
    entries_.push_back(entry);
}

void IssueQueue::remove(std::uint64_t robId) {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [robId](const IssueEntry& entry) {
        return entry.robId == robId;
    }), entries_.end());
}

void IssueQueue::flushYoungerThan(std::uint64_t robId) {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [robId](const IssueEntry& entry) {
        return entry.robId > robId;
    }), entries_.end());
}

void IssueQueue::clear() {
    entries_.clear();
}

std::size_t IssueQueue::size() const {
    return entries_.size();
}

std::size_t IssueQueue::capacity() const {
    return capacity_;
}

bool IssueQueue::empty() const {
    return entries_.empty();
}

const std::vector<IssueEntry>& IssueQueue::entries() const {
    return entries_;
}

} // namespace ooo
