#pragma once

#include "AbstractMQClientMessage.hpp"

class MQClientRequestMessage : public AbstractMQClientMessage {
  public:
    static MQClientRequestMessage deserialize(char* buffer);
    MQClientRequestMessage(int requestId, size_t fileSize)
        : requestId(requestId), fileSize(fileSize){};
    size_t getContentSize() override;
    size_t getFileSize();
    int getRequestId();

  protected:
    virtual MQClientMessageType getMessageType() override;
    void serializeContents(char* dest) override;

  private:
    int requestId;
    size_t fileSize;
};