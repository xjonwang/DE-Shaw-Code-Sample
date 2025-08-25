#pragma once

#include "AbstractMQClientRegistrationMessage.hpp"

class MQClientRegistrationMessage : public AbstractMQClientRegistrationMessage {
  public:
    static MQClientRegistrationMessage deserialize(char* buffer);
    MQClientRegistrationMessage(){};
    void accept(RegistrationVisitor&) override;
    size_t getContentSize() override;

  protected:
    virtual MQClientMessageType getMessageType() override;
    void serializeContents(char* dest) override;
};