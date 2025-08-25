#include "AbstractShmSegment.hpp"
#include "Util.hpp"

size_t AbstractShmSegment::getSize() { return size; }

void AbstractShmSegment::serialize(char* buffer) {
    std::vector<int> segmentIds = getSegmentIds();
    size_t segmentCount = segmentIds.size();
    std::copy(reinterpret_cast<char*>(segmentCount),
              reinterpret_cast<char*>(segmentCount) + sizeof(segmentCount), buffer);
    buffer += sizeof(segmentCount);
    for (size_t i = 0; i < segmentIds.size(); buffer += sizeof(segmentIds[i]), i++) {
        std::copy(reinterpret_cast<char*>(segmentIds[i]),
                  reinterpret_cast<char*>(segmentIds[i]) + sizeof(segmentIds[i]), buffer);
    }
}
