#include "MQClientRequestMessage.hpp"

MQClientRequestMessage MQClientRequestMessage::deserialize(char* buffer) {
    int messageType = static_cast<MQClientMessageType>(*reinterpret_cast<int*>(buffer));
    buffer += sizeof(messageType);
    int requestId = *reinterpret_cast<int*>(buffer);
    buffer += sizeof(requestId);
    size_t fileSize = *reinterpret_cast<size_t*>(buffer);
    return MQClientRequestMessage(requestId, fileSize);
}

int MQClientRequestMessage::getRequestId() { return requestId; }

size_t MQClientRequestMessage::getFileSize() { return fileSize; }

MQClientMessageType MQClientRequestMessage::getMessageType() {
    return MQClientMessageType::REQUEST;
}

size_t MQClientRequestMessage::getContentSize() {
    return AbstractMQClientMessage::getContentSize() + sizeof(requestId) + sizeof(fileSize);
}

void MQClientRequestMessage::serializeContents(char* dest) {
    std::copy(reinterpret_cast<const char*>(&requestId),
              reinterpret_cast<const char*>(&requestId) + sizeof(requestId), dest);
    std::copy(reinterpret_cast<const char*>(&fileSize),
              reinterpret_cast<const char*>(&fileSize) + sizeof(fileSize),
              dest + sizeof(requestId));
}
