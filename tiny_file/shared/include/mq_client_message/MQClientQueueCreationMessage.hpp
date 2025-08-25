#pragma once

#include "AbstractMQClientRegistrationMessage.hpp"
#include "CommunicationManager.hpp"

class MQClientQueueCreationMessage : public AbstractMQClientRegistrationMessage {
  public:
    static MQClientQueueCreationMessage deserialize(char* buffer);
    MQClientQueueCreationMessage(int clientId, int priority)
        : clientId(clientId), priority(priority){};
    MQClientQueueCreationMessage(int clientId)
        : MQClientQueueCreationMessage(clientId, DEFAULT_PRIORITY){};
    void accept(RegistrationVisitor&) override;
    size_t getContentSize() override;
    int getClientId();
    int getPriority();

  protected:
    virtual MQClientMessageType getMessageType() override;
    void serializeContents(char* dest) override;

  private:
    int clientId;
    int priority;
};