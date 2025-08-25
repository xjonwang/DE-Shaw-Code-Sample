#include "MQClientQueueCreationMessage.hpp"
#include "RegistrationVisitor.hpp"

MQClientQueueCreationMessage MQClientQueueCreationMessage::deserialize(char* buffer) {
    int messageType = static_cast<MQClientMessageType>(*reinterpret_cast<int*>(buffer));
    buffer += sizeof(messageType);
    int clientId = *reinterpret_cast<int*>(buffer);
    buffer += sizeof(clientId);
    int priority = *reinterpret_cast<int*>(buffer);
    return MQClientQueueCreationMessage(clientId, priority);
}

MQClientMessageType MQClientQueueCreationMessage::getMessageType() {
    return MQClientMessageType::QUEUE_CREATION;
}

void MQClientQueueCreationMessage::accept(RegistrationVisitor& visitor) {
    visitor.visitQueueCreation(*this);
}

size_t MQClientQueueCreationMessage::getContentSize() {
    return AbstractMQClientRegistrationMessage::getContentSize() + sizeof(clientId) +
           sizeof(priority);
}

int MQClientQueueCreationMessage::getClientId() { return clientId; }

int MQClientQueueCreationMessage::getPriority() { return priority; }

void MQClientQueueCreationMessage::serializeContents(char* dest) {
    std::copy(reinterpret_cast<const char*>(&clientId),
              reinterpret_cast<const char*>(&clientId) + sizeof(clientId), dest);
    dest += sizeof(clientId);
    std::copy(reinterpret_cast<const char*>(&priority),
              reinterpret_cast<const char*>(&priority) + sizeof(priority), dest);
}