#include "AbstractMQClientMessage.hpp"
#include "MQClientQueueCreationMessage.hpp"
#include "MQClientRegistrationMessage.hpp"

#include <stdexcept>

std::unique_ptr<char[]> AbstractMQClientMessage::serialize() {
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(getContentSize());
    int messageType = static_cast<int>(getMessageType());
    std::copy(reinterpret_cast<const char*>(&messageType),
              reinterpret_cast<const char*>(&messageType) + sizeof(messageType), buffer.get());
    serializeContents(buffer.get() + sizeof(messageType));
    return buffer;
}

size_t AbstractMQClientMessage::getContentSize() { return sizeof(int); }
