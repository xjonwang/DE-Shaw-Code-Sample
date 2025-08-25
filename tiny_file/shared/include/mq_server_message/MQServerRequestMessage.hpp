#pragma once

#include "AbstractMQServerMessage.hpp"
#include "AbstractShmSegment.hpp"
#include "Util.hpp"

class MQServerRequestMessage : public AbstractMQServerMessage {
  public:
    MQServerRequestMessage(int requestId, AbstractShmSegment* shmSegment)
        : requestId(requestId), shmSegment(shmSegment), shmSegmentIds(shmSegment->getSegmentIds()),
          shmSegmentCount(shmSegmentIds.size()){};
    MQServerRequestMessage(int requestId, std::vector<int>& shmSegmentIds)
        : requestId(requestId), shmSegmentIds(shmSegmentIds),
          shmSegmentCount(shmSegmentIds.size()){};
    static MQServerRequestMessage deserialize(char* buffer);
    size_t getContentSize() override;
    int getRequestId();
    const std::vector<int>& getShmSegmentIds();

  protected:
    virtual MQServerMessageType getMessageType();
    void serializeContents(char*) override;

  private:
    int requestId;
    AbstractShmSegment* shmSegment;
    std::vector<int> shmSegmentIds;
    size_t shmSegmentCount;
};