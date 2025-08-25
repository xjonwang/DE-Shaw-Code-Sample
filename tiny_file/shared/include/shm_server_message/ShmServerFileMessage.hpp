#pragma once

#include "AbstractShmServerMessage.hpp"

class ShmServerFileMessage : public AbstractShmServerMessage {
  public:
    ShmServerFileMessage(AbstractShmSegment* shmSegment)
        : AbstractShmServerMessage(shmSegment), fileSize(0){};
    ShmServerFileMessage(AbstractShmSegment* shmSegment, size_t fileSize)
        : AbstractShmServerMessage(shmSegment), fileSize(fileSize){};
    void write() override;
    void parse() override;
    size_t getFileSize();

  private:
    size_t fileSize;
};
