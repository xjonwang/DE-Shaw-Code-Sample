#pragma once

#include "ISchedulingStrategy.hpp"

/**
 * @class CreditSchedulingStrategy
 * @brief Scheduling strategy that uses credit-based allocation
 *
 * Implements a credit-based scheduling strategy for managing client requests by calling the corresponding method on the Scheduler instance.
 * Keeps ownership of all scheduling data inside the Scheduler instance.
 */
class CreditSchedulingStrategy : public ISchedulingStrategy {
  public:
    std::unique_ptr<IShmMessageHandler> schedule(Scheduler&) override;
};