#include "MQServerRequestMessage.hpp"
#include "Util.hpp"

MQServerRequestMessage MQServerRequestMessage::deserialize(char* buffer) {
    int messageType = static_cast<MQServerMessageType>(*reinterpret_cast<int*>(buffer));
    buffer += sizeof(messageType);
    int requestId = *reinterpret_cast<int*>(buffer);
    buffer += sizeof(requestId);
    size_t shmSegmentCount = *reinterpret_cast<size_t*>(buffer);
    buffer += sizeof(shmSegmentCount);
    std::vector<int> shmSegmentIds(shmSegmentCount);
    for (size_t i = 0; i < shmSegmentCount; buffer += sizeof(int), i++) {
        shmSegmentIds[i] = *reinterpret_cast<int*>(buffer);
    }
    return MQServerRequestMessage(requestId, shmSegmentIds);
}

size_t MQServerRequestMessage::getContentSize() {
    return AbstractMQServerMessage::getContentSize() + sizeof(requestId) + sizeof(size_t) +
           sizeof(int) * shmSegmentIds.size();
}

int MQServerRequestMessage::getRequestId() { return requestId; }

const std::vector<int>& MQServerRequestMessage::getShmSegmentIds() { return shmSegmentIds; }

MQServerMessageType MQServerRequestMessage::getMessageType() {
    return MQServerMessageType::RESPONSE;
}

void MQServerRequestMessage::serializeContents(char* buffer) {
    std::copy(reinterpret_cast<char*>(&requestId),
              reinterpret_cast<char*>(&requestId) + sizeof(requestId), buffer);
    buffer += sizeof(requestId);
    std::copy(reinterpret_cast<char*>(&shmSegmentCount),
              reinterpret_cast<char*>(&shmSegmentCount) + sizeof(shmSegmentCount), buffer);
    buffer += sizeof(shmSegmentCount);
    for (size_t i = 0; i < shmSegmentCount; buffer += sizeof(int), i++) {
        std::copy(reinterpret_cast<char*>(&shmSegmentIds[i]),
                  reinterpret_cast<char*>(&shmSegmentIds[i]) + sizeof(shmSegmentIds[i]), buffer);
    }
}