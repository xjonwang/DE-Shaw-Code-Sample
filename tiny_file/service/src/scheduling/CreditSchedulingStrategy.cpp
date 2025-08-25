#include "CreditSchedulingStrategy.hpp"

#include "Scheduler.hpp"

/**
 * @brief Delegates to corresponding scheduling method in Scheduler
 *
 * @param scheduler Reference to the Scheduler instance
 */
std::unique_ptr<IShmMessageHandler> CreditSchedulingStrategy::schedule(Scheduler& scheduler) {
    return scheduler.scheduleCredit();
}
