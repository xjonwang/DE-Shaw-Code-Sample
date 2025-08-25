#include "ShmManager.hpp"

#include "ContiguousShmSegment.hpp"
#include "FragmentedShmSegment.hpp"
#include "Util.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

std::unique_ptr<ShmManager> ShmManager::instance;
std::once_flag ShmManager::onceFlag;

ShmManager::ShmManager(size_t numShmSegments, size_t shmSegmentSize, size_t internalRepSize,
                       bool create)
    : numSegments(numShmSegments), physicalSegmentSize(shmSegmentSize),
      internalRepSize(internalRepSize), size(numShmSegments * shmSegmentSize), availableSize(size),
      numInternalSegments(numShmSegments * shmSegmentSize / internalRepSize),
      shmAddresses(numShmSegments), availableSegments(numInternalSegments, true) {
    shmSegments.reserve(numInternalSegments);
    int internalShmSegmentId = 0;
    for (size_t shmSegmentId = 0; shmSegmentId < numShmSegments; shmSegmentId++) {
        if (create) {
            shm_unlink(communicationManager.getShmSegmentName(shmSegmentId).c_str());
        }
        int shmFileDescriptor =
            shm_open(communicationManager.getShmSegmentName(shmSegmentId).c_str(),
                     create ? O_CREAT | O_EXCL | O_RDWR : O_RDWR,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (shmFileDescriptor == -1) {
            THROW_RUNTIME_ERROR("ShmManager: shm_open failed");
        }

        if (create && ftruncate(shmFileDescriptor, shmSegmentSize) == -1) {
            close(shmFileDescriptor);
            THROW_RUNTIME_ERROR("ShmManager: ftruncate failed");
        }

        shmAddresses[shmSegmentId] =
            mmap(nullptr, shmSegmentSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFileDescriptor, 0);
        if (shmAddresses[shmSegmentId] == MAP_FAILED) {
            close(shmFileDescriptor);
            THROW_RUNTIME_ERROR("ShmManager: mmap failed");
        }

        char* sharedAddress = static_cast<char*>(shmAddresses[shmSegmentId]);
        for (size_t internalSegmentOffset = 0; internalSegmentOffset < shmSegmentSize;
             internalSegmentOffset += internalRepSize, internalShmSegmentId++) {
            shmSegments.emplace_back(internalShmSegmentId, sharedAddress + internalSegmentOffset,
                                     internalRepSize);
        }
    }
}

ShmManager::~ShmManager() {
    for (size_t shmSegmentId = 0; shmSegmentId < numSegments; shmSegmentId++) {
        munmap(shmAddresses[shmSegmentId], physicalSegmentSize);
    }
}

size_t ShmManager::getNumShmSegments() { return numSegments; }

size_t ShmManager::getShmSegmentSize() { return physicalSegmentSize; }

size_t ShmManager::getInternalRepSize() { return internalRepSize; }

size_t ShmManager::getSize() { return size; }

size_t ShmManager::getAvailableSize() { return availableSize; }

std::unique_ptr<AbstractShmSegment> ShmManager::getShmSegment(size_t requestSize) {
    if (requestSize % internalRepSize) {
        THROW_RUNTIME_ERROR("ShmManager: size not multiple of internal segment size");
    }

    std::unique_lock<std::mutex> lock(segmentMutex);
    segmentCond.wait(lock, [this, requestSize] { return availableSize >= requestSize; });

    std::vector<std::reference_wrapper<AbstractShmSegment>> retSegments;
    for (size_t i = 0; i < numInternalSegments && requestSize > 0; i++) {
        if (availableSegments[i]) {
            retSegments.push_back(std::ref(shmSegments[i]));
            availableSize -= shmSegments[i].getSize();
            requestSize -= shmSegments[i].getSize();
            availableSegments[i] = false;
        }
    }
    return std::make_unique<FragmentedShmSegment>(retSegments);
}

std::unique_ptr<AbstractShmSegment> ShmManager::getShmSegment(const std::vector<int>& shmIds) {
    std::vector<std::reference_wrapper<AbstractShmSegment>> retSegments;
    retSegments.reserve(shmIds.size());
    for (int id : shmIds) {
        if (!availableSegments[id]) {
            THROW_RUNTIME_ERROR("ShmManager: segment " + std::to_string(id) + " already allocated");
        }
        availableSegments[id] = false;
        retSegments.push_back(std::ref(shmSegments[id]));
    }
    return std::make_unique<FragmentedShmSegment>(retSegments);
}

void ShmManager::releaseShmSegment(int id) {
    std::lock_guard<std::mutex> lock(segmentMutex);
    availableSegments[id] = true;
    availableSize += shmSegments[id].getSize();
    segmentCond.notify_one();
}

void ShmManager::releaseShmSegment(const std::vector<int>& ids) {
    std::lock_guard<std::mutex> lock(segmentMutex);
    for (const int& id : ids) {
        availableSegments[id] = true;
        availableSize += shmSegments[id].getSize();
    }
    segmentCond.notify_one();
}