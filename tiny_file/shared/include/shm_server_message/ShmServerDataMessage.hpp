#pragma once

#include "AbstractShmServerMessage.hpp"

class ShmServerDataMessage : public AbstractShmServerMessage {
  public:
    static size_t headerSize();
    ShmServerDataMessage(AbstractShmSegment* shmSegment)
        : AbstractShmServerMessage(shmSegment), data(nullptr), dataSize(0){};
    ShmServerDataMessage(AbstractShmSegment* shmSegment, char* data, size_t dataSize)
        : AbstractShmServerMessage(shmSegment), data(data), dataSize(dataSize){};
    void write() override;
    void parse() override;
    size_t getDataSize();
    void copyData(char* dest, size_t bufferSize);

  private:
    char* data;
    size_t dataSize;
};
