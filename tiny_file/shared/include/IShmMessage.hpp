#pragma once

#include <memory>

class IShmMessage {
  public:
    virtual void write() = 0;
    virtual void parse() = 0;
    virtual ~IShmMessage() = default;
};