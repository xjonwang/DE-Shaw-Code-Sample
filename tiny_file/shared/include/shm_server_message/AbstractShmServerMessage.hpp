#pragma once

#include "AbstractShmSegment.hpp"
#include "IShmMessage.hpp"

class AbstractShmServerMessage : public IShmMessage {
  public:
    AbstractShmServerMessage(AbstractShmSegment* shmSegment) : shmSegment(shmSegment){};

  protected:
    AbstractShmSegment* shmSegment;
};