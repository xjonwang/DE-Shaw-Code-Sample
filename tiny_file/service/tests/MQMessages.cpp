#include <gtest/gtest.h>

#include "MQClientQueueCreationMessage.hpp"

TEST(MQClientQueueCreationMessage, full) {
    MQClientQueueCreationMessage sendMessage(1, 2);
    std::unique_ptr<char[]> buffer = sendMessage.serialize();
    MQClientQueueCreationMessage recvMessage =
        MQClientQueueCreationMessage::deserialize(buffer.get());
    EXPECT_EQ(recvMessage.getClientId(), 1);
    EXPECT_EQ(recvMessage.getPriority(), 2);

    MQClientQueueCreationMessage sendMessage2(1);
    std::unique_ptr<char[]> buffer2 = sendMessage2.serialize();
    MQClientQueueCreationMessage recvMessage2 =
        MQClientQueueCreationMessage::deserialize(buffer2.get());
    EXPECT_EQ(recvMessage2.getClientId(), 1);
    EXPECT_EQ(recvMessage2.getPriority(), 64);
}