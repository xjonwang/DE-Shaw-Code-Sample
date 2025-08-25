#pragma once

#include <stddef.h>
#include <vector>

class AbstractShmSegment {
  public:
    AbstractShmSegment(size_t size) : size(size){};
    virtual void write(const char*, size_t, size_t = 0) = 0;
    virtual void read(char*, size_t, size_t = 0) = 0;
    void serialize(char*);
    size_t getSize();
    virtual std::vector<int> getSegmentIds() = 0;
    virtual ~AbstractShmSegment() = default;

  protected:
    size_t size;

  private:
    char* addr;
};