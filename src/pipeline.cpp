#include "pipeline.hpp"

namespace ooo {

PipelineQueues::PipelineQueues(std::size_t queueCapacity) : queueCapacity_(queueCapacity) {}

void PipelineQueues::clear() {
    fetch.clear();
    decode.clear();
    rename.clear();
}

bool PipelineQueues::empty() const {
    return fetch.empty() && decode.empty() && rename.empty();
}

bool PipelineQueues::fetchCanAccept() const {
    return fetch.size() < queueCapacity_;
}

bool PipelineQueues::decodeCanAccept() const {
    return decode.size() < queueCapacity_;
}

bool PipelineQueues::renameCanAccept() const {
    return rename.size() < queueCapacity_;
}

std::size_t PipelineQueues::queueCapacity() const {
    return queueCapacity_;
}

} // namespace ooo
