#pragma once

#include "AbstractShmSegment.hpp"

class ContiguousShmSegment : public AbstractShmSegment {
  public:
    ContiguousShmSegment(int id, char* addr, size_t size)
        : AbstractShmSegment(size), id(id), addr(addr){};
    void write(const char*, size_t, size_t) override;
    void read(char*, size_t, size_t) override;
    std::vector<int> getSegmentIds() override;
    ~ContiguousShmSegment() override;

  private:
    int id;
    char* addr;
};
