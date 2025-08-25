#pragma once

#include "AbstractShmSegment.hpp"
#include <functional>
#include <vector>

class FragmentedShmSegment : public AbstractShmSegment {
  public:
    FragmentedShmSegment(
        const std::vector<std::reference_wrapper<AbstractShmSegment>>& shmSegments);
    void write(const char*, size_t, size_t) override;
    void read(char*, size_t, size_t) override;
    std::vector<int> getSegmentIds();
    ~FragmentedShmSegment() override;

  private:
    std::vector<std::reference_wrapper<AbstractShmSegment>> shmSegments;
};
