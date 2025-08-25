#pragma once

#include <memory>
#include <stddef.h>

class IMQMessage {
  public:
    virtual std::unique_ptr<char[]> serialize() = 0;
    virtual size_t getContentSize() = 0;
    virtual ~IMQMessage() = default;
};