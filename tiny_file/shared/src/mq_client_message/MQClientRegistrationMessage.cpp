#include "MQClientRegistrationMessage.hpp"
#include "RegistrationVisitor.hpp"

MQClientRegistrationMessage MQClientRegistrationMessage::deserialize(char* /*buffer*/) {
    return MQClientRegistrationMessage();
}

MQClientMessageType MQClientRegistrationMessage::getMessageType() {
    return MQClientMessageType::REGISTRATION;
}

void MQClientRegistrationMessage::accept(RegistrationVisitor& visitor) {
    visitor.visitRegistration(*this);
}

size_t MQClientRegistrationMessage::getContentSize() {
    return AbstractMQClientRegistrationMessage::getContentSize();
}

void MQClientRegistrationMessage::serializeContents(char* /*dest*/) {
    // No contents to serialize
}