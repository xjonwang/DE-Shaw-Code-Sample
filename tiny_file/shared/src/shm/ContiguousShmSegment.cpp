#include "ContiguousShmSegment.hpp"
#include "ShmManager.hpp"

#include <algorithm>
#include <stdexcept>

void ContiguousShmSegment::write(const char* data, size_t dataSize, size_t offset) {
    if (offset + dataSize <= size) {
        std::copy(data, data + dataSize, addr + offset);
    } else {
        THROW_RUNTIME_ERROR("ContiguousShmSegment: data size exceeded in write");
    }
}

void ContiguousShmSegment::read(char* data, size_t dataSize, size_t offset) {
    if (offset + dataSize <= size) {
        std::copy(addr + offset, addr + offset + dataSize, data);
    } else {
        THROW_RUNTIME_ERROR("ContiguousShmSegment: data size exceeded in read");
    }
}

std::vector<int> ContiguousShmSegment::getSegmentIds() { return {id}; }

ContiguousShmSegment::~ContiguousShmSegment() { ShmManager::getInstance().releaseShmSegment(id); }