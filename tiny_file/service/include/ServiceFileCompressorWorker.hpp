#pragma once

#include "Scheduler.hpp"

/**
 * @class ServiceFileCompressorWorker
 * @brief Handles the compression of files.
 *
 * Spins in infinite loop waiting for scheduler to dispatch requests and processes them.
 */
class ServiceFileCompressorWorker {
  public:
    ServiceFileCompressorWorker(int workerId) : workerId(workerId){};
    void run();

  private:
    int workerId;
    Scheduler& scheduler = Scheduler::getInstance();
};