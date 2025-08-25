#pragma once

#include "AbstractShmClientMessage.hpp"

class ShmClientDataMessage : public AbstractShmClientMessage {
  public:
    static size_t headerSize();
    ShmClientDataMessage(AbstractShmSegment* shmSegment)
        : AbstractShmClientMessage(shmSegment), data(nullptr), dataSize(0){};
    ShmClientDataMessage(AbstractShmSegment* shmSegment, char* data, size_t dataSize)
        : AbstractShmClientMessage(shmSegment), data(data), dataSize(dataSize){};
    void write() override;
    void parse() override;
    size_t getDataSize();
    void copyData(char* dest, size_t bufferSize);

  private:
    char* data;
    size_t dataSize;
};
