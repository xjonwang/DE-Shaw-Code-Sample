#pragma once

#include "IShmMessageHandler.hpp"
#include <memory>

class Scheduler;

/**
 * @class ISchedulingStrategy
 * @brief Interface for scheduling strategies
 *
 * Defines the Strategy pattern interface used by the Scheduler to determine the order in which client requests are processed.
 *
 * @param scheduler Reference to the Scheduler instance
 */
class ISchedulingStrategy {
  public:
    virtual std::unique_ptr<IShmMessageHandler> schedule(Scheduler&) = 0;
    virtual ~ISchedulingStrategy() = default;
};