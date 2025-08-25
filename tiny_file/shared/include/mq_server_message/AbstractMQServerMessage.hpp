#pragma once

#include "IMQMessage.hpp"
#include "MQServerMessageType.hpp"

class AbstractMQServerMessage : public IMQMessage {
  public:
    AbstractMQServerMessage(){};
    std::unique_ptr<char[]> serialize() override;
    virtual size_t getContentSize() override;

  protected:
    virtual MQServerMessageType getMessageType() = 0;
    virtual void serializeContents(char*) = 0;
};