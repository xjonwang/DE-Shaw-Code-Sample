#include <gtest/gtest.h>

#include "AbstractShmSegment.hpp"
#include "ShmManager.hpp"

TEST(ShmManagerTest, initialize) {
    ShmManager::init(10, 1024, 32, true);
    ShmManager& manager = ShmManager::getInstance();
    EXPECT_EQ(manager.getSize(), 10240);
    EXPECT_EQ(manager.getAvailableSize(), 10240);
    EXPECT_EQ(manager.numSegments, 10);
    EXPECT_EQ(manager.numInternalSegments, 320);
    EXPECT_EQ(manager.internalRepSize, 32);
    std::unique_ptr<AbstractShmSegment> segment = manager.getShmSegment(64);
    EXPECT_EQ(manager.getAvailableSize(), 10176);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(manager.availableSegments[i], false);
    }
    std::unique_ptr<AbstractShmSegment> segment2 = manager.getShmSegment(64);
    EXPECT_EQ(manager.getAvailableSize(), 10112);
    for (int i = 2; i < 4; i++) {
        EXPECT_EQ(manager.availableSegments[i], false);
    }
    std::unique_ptr<AbstractShmSegment> segment3 = manager.getShmSegment(1024);
    EXPECT_EQ(manager.getAvailableSize(), 9088);
    for (int i = 4; i < 36; i++) {
        EXPECT_EQ(manager.availableSegments[i], false);
    }
    for (int i = 36; i < 320; i++) {
        EXPECT_EQ(manager.availableSegments[i], true);
    }
    EXPECT_THROW(manager.getShmSegment(33), std::runtime_error);
    segment.reset();
    segment2.reset();
    segment3.reset();
    EXPECT_EQ(manager.getAvailableSize(), 10240);
    for (int i = 0; i < 320; i++) {
        EXPECT_EQ(manager.availableSegments[i], true);
    }
}

TEST(ShmSegmentTest, full) {
    ShmManager::init(10, 1024, 32, true);
    ShmManager& manager = ShmManager::getInstance();
    // ContiguousShmSegment
    std::unique_ptr<AbstractShmSegment> contiguousSegment = manager.getShmSegment(32);
    contiguousSegment->write("Hello, World!", 14);
    std::unique_ptr<char[]> contiguousShmBuffer = std::make_unique<char[]>(18);
    contiguousSegment->read(contiguousShmBuffer.get(), 14);
    EXPECT_EQ(std::string(contiguousShmBuffer.get()), "Hello, World!");
    contiguousSegment->write("Hello, World!", 14, 4);
    contiguousSegment->read(contiguousShmBuffer.get(), 14, 4);
    EXPECT_EQ(std::string(contiguousShmBuffer.get()), "Hello, World!");
    contiguousSegment->read(contiguousShmBuffer.get(), 18);
    EXPECT_EQ(std::string(contiguousShmBuffer.get()), "HellHello, World!");
    // FragmentedShmSegment
    std::unique_ptr<AbstractShmSegment> fragmentedSegment = manager.getShmSegment(64);
    fragmentedSegment->write("Really long message that goes past the 32 byte boundary", 56);
    std::unique_ptr<char[]> fragmentedShmBuffer = std::make_unique<char[]>(60);
    fragmentedSegment->read(fragmentedShmBuffer.get(), 56);
    EXPECT_EQ(std::string(fragmentedShmBuffer.get()),
              "Really long message that goes past the 32 byte boundary");
    fragmentedSegment->write("Really long message that goes past the 32 byte boundary", 56, 4);
    fragmentedSegment->read(fragmentedShmBuffer.get(), 56, 4);
    EXPECT_EQ(std::string(fragmentedShmBuffer.get()),
              "Really long message that goes past the 32 byte boundary");
    fragmentedSegment->read(fragmentedShmBuffer.get(), 60);
    EXPECT_EQ(std::string(fragmentedShmBuffer.get()),
              "RealReally long message that goes past the 32 byte boundary");
    contiguousSegment.reset();
    fragmentedSegment.reset();
}