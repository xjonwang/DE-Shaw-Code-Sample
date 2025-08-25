#pragma once

#include "AbstractMQClientRegistrationMessage.hpp"

class MQClientQueueDeletionMessage : public AbstractMQClientRegistrationMessage {
  public:
    static MQClientQueueDeletionMessage deserialize(char* buffer);
    MQClientQueueDeletionMessage(int clientId) : clientId(clientId){};
    void accept(RegistrationVisitor&) override;
    size_t getContentSize() override;
    int getClientId();

  protected:
    virtual MQClientMessageType getMessageType() override;
    void serializeContents(char* dest) override;

  private:
    int clientId;
};