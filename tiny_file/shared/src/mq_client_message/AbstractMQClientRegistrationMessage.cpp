#include "AbstractMQClientRegistrationMessage.hpp"
#include "MQClientQueueCreationMessage.hpp"
#include "MQClientQueueDeletionMessage.hpp"
#include "MQClientRegistrationMessage.hpp"
#include "Util.hpp"

#include <stdexcept>

std::unique_ptr<AbstractMQClientRegistrationMessage>
AbstractMQClientRegistrationMessage::deserialize(char* buffer) {
    MQClientMessageType messageType =
        static_cast<MQClientMessageType>(*reinterpret_cast<int*>(buffer));
    switch (messageType) {
    case MQClientMessageType::REGISTRATION:
        return std::make_unique<MQClientRegistrationMessage>(
            MQClientRegistrationMessage::deserialize(buffer));
    case MQClientMessageType::QUEUE_CREATION:
        return std::make_unique<MQClientQueueCreationMessage>(
            MQClientQueueCreationMessage::deserialize(buffer));
    case MQClientMessageType::QUEUE_DELETION:
        return std::make_unique<MQClientQueueDeletionMessage>(
            MQClientQueueDeletionMessage::deserialize(buffer));
    default:
        THROW_RUNTIME_ERROR("AbstractMQClientRegistrationMessage: unknown message type");
    }
}