#pragma once

#include "AbstractShmSegment.hpp"
#include "CommunicationManager.hpp"
#include "ContiguousShmSegment.hpp"
#include "Util.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <vector>

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

class ShmManager {
  public:
    static void init(size_t numShmSegments, size_t shmSegmentSize, size_t internalRepSize,
                     bool create = false) {
        std::call_once(onceFlag, [numShmSegments, shmSegmentSize, internalRepSize, create] {
            instance.reset(new ShmManager(numShmSegments, shmSegmentSize, internalRepSize, create));
        });
    };
    static ShmManager& getInstance() {
        if (!instance) {
            THROW_RUNTIME_ERROR("ShmManager: instance not initialized");
        }
        return *instance;
    };
    ~ShmManager();
    size_t getNumShmSegments();
    size_t getShmSegmentSize();
    size_t getInternalRepSize();
    size_t getSize();
    size_t getAvailableSize();
    std::unique_ptr<AbstractShmSegment> getShmSegment(size_t size);
    std::unique_ptr<AbstractShmSegment> getShmSegment(const std::vector<int>& shmIds);
    void releaseShmSegment(int);
    void releaseShmSegment(const std::vector<int>&);

  private:
    ShmManager(size_t, size_t, size_t, bool);
    ShmManager(ShmManager const&) = delete;
    void operator=(ShmManager const&) = delete;
    static std::unique_ptr<ShmManager> instance;
    static std::once_flag onceFlag;
    CommunicationManager& communicationManager = CommunicationManager::getInstance();
    size_t numSegments;                  // number of physical segments
    size_t physicalSegmentSize;          // size of physical segments
    size_t internalRepSize;              // size of virtual segments
    size_t size;                         // total size of all segments
    size_t availableSize;                // total free size of all segments
    size_t numInternalSegments;          // number of virtual segments
    std::vector<void*> shmAddresses;     // addresses of physical segments
    std::vector<bool> availableSegments; // availability of virtual segments
    std::mutex segmentMutex;
    std::condition_variable segmentCond;
    std::vector<ContiguousShmSegment> shmSegments; // virtual segments

#ifdef UNIT_TEST
    FRIEND_TEST(ShmManagerTest, initialize);
#endif
};