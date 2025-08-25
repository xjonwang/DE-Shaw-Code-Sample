#pragma once

#include "AbstractShmSegment.hpp"
#include "IShmMessage.hpp"

class AbstractShmClientMessage : public IShmMessage {
  public:
    AbstractShmClientMessage(AbstractShmSegment* shmSegment) : shmSegment(shmSegment){};

  protected:
    AbstractShmSegment* shmSegment;
};