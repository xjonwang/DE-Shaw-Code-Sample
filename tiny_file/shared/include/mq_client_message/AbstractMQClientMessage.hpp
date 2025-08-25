#pragma once

#include "IMQMessage.hpp"
#include "MQClientMessageType.hpp"

class AbstractMQClientMessage : public IMQMessage {
  public:
    AbstractMQClientMessage(){};
    std::unique_ptr<char[]> serialize() override;
    size_t getContentSize() override;

  protected:
    virtual MQClientMessageType getMessageType() = 0;
    virtual void serializeContents(char* dest) = 0;
};