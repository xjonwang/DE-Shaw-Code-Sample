#pragma once

#include "AbstractMQClientMessage.hpp"

class RegistrationVisitor;

class AbstractMQClientRegistrationMessage : public AbstractMQClientMessage {
  public:
    static std::unique_ptr<AbstractMQClientRegistrationMessage> deserialize(char* buffer);
    virtual void accept(RegistrationVisitor&) = 0;
};