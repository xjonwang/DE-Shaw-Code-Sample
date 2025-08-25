#pragma once

#include "ISchedulingStrategy.hpp"

/**
 * @class RoundRobinSchedulingStrategy
 * @brief Scheduling strategy that uses round-robin allocation
 *
 * Implements a round-robin scheduling strategy for managing client requests by calling the corresponding method on the Scheduler instance.
 * Keeps ownership of all scheduling data inside the Scheduler instance.
 */
class RoundRobinSchedulingStrategy : public ISchedulingStrategy {
  public:
    std::unique_ptr<IShmMessageHandler> schedule(Scheduler&) override;
};