#include <gtest/gtest.h>

#include "AbstractShmSegment.hpp"
#include "ShmClientDataMessage.hpp"
#include "ShmManager.hpp"
#include "ShmServerDataMessage.hpp"
#include "ShmServerFileMessage.hpp"

TEST(ShmServerDataMessage, full) {
    ShmManager::init(10, 1024, 32, true);
    ShmManager& manager = ShmManager::getInstance();
    std::string msg = "Hello, World!";
    std::unique_ptr<AbstractShmSegment> segment = manager.getShmSegment(64);
    ShmServerDataMessage sendMessage(segment.get(), msg.data(), msg.size());
    sendMessage.write();
    ShmServerDataMessage recvMessage(segment.get());
    recvMessage.parse();
    std::unique_ptr<char[]> recvBuffer = std::make_unique<char[]>(msg.size());
    recvMessage.copyData(recvBuffer.get(), msg.size());
    EXPECT_EQ(std::string(recvBuffer.get()), msg);
}

TEST(ShmClientDataMessage, full) {
    ShmManager::init(10, 1024, 32, true);
    ShmManager& manager = ShmManager::getInstance();
    std::string msg = "Hello, World!";
    std::unique_ptr<AbstractShmSegment> segment = manager.getShmSegment(64);
    ShmClientDataMessage sendMessage(segment.get(), msg.data(), msg.size());
    sendMessage.write();
    ShmClientDataMessage recvMessage(segment.get());
    recvMessage.parse();
    std::unique_ptr<char[]> recvBuffer = std::make_unique<char[]>(msg.size());
    recvMessage.copyData(recvBuffer.get(), msg.size());
    EXPECT_EQ(std::string(recvBuffer.get()), msg);
}

TEST(ShmServerFileMessage, full) {
    ShmManager::init(10, 1024, 32, true);
    ShmManager& manager = ShmManager::getInstance();
    std::unique_ptr<AbstractShmSegment> segment = manager.getShmSegment(64);

    ShmServerFileMessage sendMessage(segment.get(), 42);
    sendMessage.write();
    ShmServerFileMessage recvMessage(segment.get());
    recvMessage.parse();
    EXPECT_EQ(recvMessage.getFileSize(), 42);

    sendMessage = ShmServerFileMessage(segment.get(), 100);
    sendMessage.write();
    recvMessage = ShmServerFileMessage(segment.get());
    recvMessage.parse();
    EXPECT_EQ(recvMessage.getFileSize(), 100);

    sendMessage = ShmServerFileMessage(segment.get(), 1000000000);
    sendMessage.write();
    recvMessage = ShmServerFileMessage(segment.get());
    recvMessage.parse();
    EXPECT_EQ(recvMessage.getFileSize(), 1000000000);
}