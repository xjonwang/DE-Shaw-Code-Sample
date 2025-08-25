#pragma once

#include "ShmClientDataMessage.hpp"

class IShmMessageHandler {
  public:
    virtual void process() = 0;
    virtual ~IShmMessageHandler() = default;
};