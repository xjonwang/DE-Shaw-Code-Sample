#include "MQServerIdMessage.hpp"

MQServerIdMessage MQServerIdMessage::deserialize(char* buffer) {
    int messageType = static_cast<MQServerMessageType>(*reinterpret_cast<int*>(buffer));
    buffer += sizeof(messageType);
    int clientId = *reinterpret_cast<int*>(buffer);
    buffer += sizeof(clientId);
    size_t numShmSegments = *reinterpret_cast<size_t*>(buffer);
    buffer += sizeof(numShmSegments);
    size_t shmSegmentSize = *reinterpret_cast<size_t*>(buffer);
    buffer += sizeof(shmSegmentSize);
    size_t internalRepSize = *reinterpret_cast<size_t*>(buffer);
    return MQServerIdMessage(clientId, numShmSegments, shmSegmentSize, internalRepSize);
}

size_t MQServerIdMessage::getContentSize() {
    return AbstractMQServerMessage::getContentSize() + sizeof(clientId) + sizeof(numShmSegments) +
           sizeof(shmSegmentSize) + sizeof(internalRepSize);
}

MQServerMessageType MQServerIdMessage::getMessageType() { return MQServerMessageType::ID; }

int MQServerIdMessage::getClientId() { return clientId; }

size_t MQServerIdMessage::getNumShmSegments() { return numShmSegments; }

size_t MQServerIdMessage::getShmSegmentSize() { return shmSegmentSize; }

size_t MQServerIdMessage::getInternalRepSize() { return internalRepSize; }

void MQServerIdMessage::serializeContents(char* buffer) {
    std::copy(reinterpret_cast<char*>(&clientId),
              reinterpret_cast<char*>(&clientId) + sizeof(clientId), buffer);
    buffer += sizeof(clientId);
    std::copy(reinterpret_cast<char*>(&numShmSegments),
              reinterpret_cast<char*>(&numShmSegments) + sizeof(numShmSegments), buffer);
    buffer += sizeof(numShmSegments);
    std::copy(reinterpret_cast<char*>(&shmSegmentSize),
              reinterpret_cast<char*>(&shmSegmentSize) + sizeof(shmSegmentSize), buffer);
    buffer += sizeof(shmSegmentSize);
    std::copy(reinterpret_cast<char*>(&internalRepSize),
              reinterpret_cast<char*>(&internalRepSize) + sizeof(internalRepSize), buffer);
}