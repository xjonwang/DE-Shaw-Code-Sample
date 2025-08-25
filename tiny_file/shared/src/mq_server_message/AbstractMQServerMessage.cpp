#include "AbstractMQServerMessage.hpp"

size_t AbstractMQServerMessage::getContentSize() { return sizeof(int); }

std::unique_ptr<char[]> AbstractMQServerMessage::serialize() {
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(getContentSize());
    int messageType = getMessageType();
    std::copy(reinterpret_cast<const char*>(&messageType),
              reinterpret_cast<const char*>(&messageType) + sizeof(messageType), buffer.get());
    serializeContents(buffer.get() + sizeof(messageType));
    return buffer;
}