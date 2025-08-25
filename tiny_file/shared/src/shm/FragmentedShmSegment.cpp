#include "FragmentedShmSegment.hpp"
#include "ShmManager.hpp"
#include "Util.hpp"

#include <algorithm>
#include <stdexcept>

FragmentedShmSegment::FragmentedShmSegment(
    const std::vector<std::reference_wrapper<AbstractShmSegment>>& shmSegments)
    : AbstractShmSegment(0), shmSegments(shmSegments) {
    for (auto& segment : shmSegments) {
        size += segment.get().getSize();
    }
}

void FragmentedShmSegment::write(const char* data, size_t dataSize, size_t offset) {
    if (offset + dataSize <= size) {
        size_t dataOffset = 0;
        for (auto& segmentRef : shmSegments) {
            if (dataSize == 0) {
                break;
            }
            AbstractShmSegment& segment = segmentRef.get();
            if (offset >= segment.getSize()) {
                offset -= segment.getSize();
            } else if (offset > 0) {
                size_t writeSize = std::min(segment.getSize() - offset, dataSize);
                segment.write(data + dataOffset, writeSize, offset);
                dataSize -= writeSize;
                dataOffset += writeSize;
                offset = 0;
            } else {
                size_t writeSize = std::min(segment.getSize(), dataSize);
                segment.write(data + dataOffset, writeSize);
                dataSize -= writeSize;
                dataOffset += writeSize;
            }
        }
    } else {
        THROW_RUNTIME_ERROR("FragmentedShmSegment: data size exceeded in write: offset " +
                            std::to_string(offset) + ", dataSize " + std::to_string(dataSize) +
                            ", size " + std::to_string(size));
    }
}

void FragmentedShmSegment::read(char* data, size_t dataSize, size_t offset) {
    if (offset + dataSize <= size) {
        size_t dataOffset = 0;
        for (auto& segmentRef : shmSegments) {
            if (dataSize == 0) {
                break;
            }
            AbstractShmSegment& segment = segmentRef.get();
            if (offset >= segment.getSize()) {
                offset -= segment.getSize();
            } else if (offset > 0) {
                size_t readSize = std::min(segment.getSize() - offset, dataSize);
                segment.read(data + dataOffset, readSize, offset);
                dataSize -= readSize;
                dataOffset += readSize;
                offset = 0;
            } else {
                size_t readSize = std::min(segment.getSize(), dataSize);
                segment.read(data + dataOffset, readSize);
                dataSize -= readSize;
                dataOffset += readSize;
            }
        }
    } else {
        THROW_RUNTIME_ERROR("FragmentedShmSegment: data size exceeded in read: offset " +
                            std::to_string(offset) + ", dataSize " + std::to_string(dataSize) +
                            ", size " + std::to_string(size));
    }
}

std::vector<int> FragmentedShmSegment::getSegmentIds() {
    std::vector<int> segmentIds;
    for (AbstractShmSegment& s : shmSegments) {
        std::vector<int> subSegmentIds = s.getSegmentIds();
        segmentIds.insert(segmentIds.end(), subSegmentIds.begin(), subSegmentIds.end());
    }
    return segmentIds;
}

FragmentedShmSegment::~FragmentedShmSegment() {
    ShmManager::getInstance().releaseShmSegment(getSegmentIds());
}
