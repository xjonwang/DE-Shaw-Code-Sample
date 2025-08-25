#include "MQClientQueueDeletionMessage.hpp"
#include "RegistrationVisitor.hpp"

MQClientQueueDeletionMessage MQClientQueueDeletionMessage::deserialize(char* buffer) {
    int messageType = static_cast<MQClientMessageType>(*reinterpret_cast<int*>(buffer));
    buffer += sizeof(messageType);
    int clientId = *reinterpret_cast<int*>(buffer);
    return MQClientQueueDeletionMessage(clientId);
}

MQClientMessageType MQClientQueueDeletionMessage::getMessageType() {
    return MQClientMessageType::QUEUE_DELETION;
}

void MQClientQueueDeletionMessage::accept(RegistrationVisitor& visitor) {
    visitor.visitQueueDeletion(*this);
}

size_t MQClientQueueDeletionMessage::getContentSize() {
    return AbstractMQClientRegistrationMessage::getContentSize() + sizeof(clientId);
}

int MQClientQueueDeletionMessage::getClientId() { return clientId; }

void MQClientQueueDeletionMessage::serializeContents(char* dest) {
    std::copy(reinterpret_cast<const char*>(&clientId),
              reinterpret_cast<const char*>(&clientId) + sizeof(clientId), dest);
}