#pragma once

#include "AbstractMQServerMessage.hpp"

class MQServerIdMessage : public AbstractMQServerMessage {
  public:
    static MQServerIdMessage deserialize(char* buffer);
    MQServerIdMessage(int clientId, size_t numShmSegments, size_t shmSegmentSize,
                      size_t internalRepSize)
        : clientId(clientId), numShmSegments(numShmSegments), shmSegmentSize(shmSegmentSize),
          internalRepSize(internalRepSize){};
    size_t getContentSize() override;
    int getClientId();
    size_t getNumShmSegments();
    size_t getShmSegmentSize();
    size_t getInternalRepSize();

  protected:
    MQServerMessageType getMessageType() override;
    void serializeContents(char* buffer) override;

  private:
    int clientId;
    size_t numShmSegments;
    size_t shmSegmentSize;
    size_t internalRepSize;
};