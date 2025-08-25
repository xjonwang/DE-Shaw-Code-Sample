#include "Scheduler.hpp"
#include "ServiceFileCompressorHandler.hpp"
#include "CommunicationManager.hpp"
#include "Util.hpp"

/**
 * @brief Enqueues client into appropriate credit queue
 *
 * If client still has positive credit, it is enqueued in the underQueue.
 * If client has no credit, it is enqueued in the overQueue.
 *
 * @param scheduler Reference to the Scheduler instance
 */
void Scheduler::Credit::enqueue(Scheduler& scheduler) {
    if (type == CreditType::IDLE) {
        if (credit > 0) {
            type = CreditType::UNDER;
            scheduler.underQueue.push_back(clientId);
            iterator = std::prev(scheduler.underQueue.end());
        } else {
            type = CreditType::OVER;
            scheduler.overQueue.push_back(clientId);
            iterator = std::prev(scheduler.overQueue.end());
        }
    }
}

/**
 * @brief Dequeues client from its current credit queue
 *
 * @param scheduler Reference to the Scheduler instance
 */
void Scheduler::Credit::dequeue(Scheduler& scheduler) {
    switch (type) {
    case CreditType::UNDER:
        scheduler.underQueue.erase(iterator);
        break;
    case CreditType::OVER:
        scheduler.overQueue.erase(iterator);
        break;
    default:
        THROW_RUNTIME_ERROR("Scheduler: schedule called on idle Credit");
        break;
    }
}

/**
 * @brief Schedules a client request.
 * 
 * Selects first request from client's request queue and schedules it.
 * Updates all clients' credits based on file size and priority.
 * 
 * @return The scheduled client request.
 */
Scheduler::Request Scheduler::Credit::schedule(Scheduler& scheduler) {
    if (scheduler.requestMap[clientId].empty()) {
        THROW_RUNTIME_ERROR("Scheduler: schedule called on Credit with empty runqueue");
    }
    Request request = scheduler.requestMap[clientId].front();
    scheduler.requestMap[clientId].pop_front();
    dequeue(scheduler);
    for (auto& [otherClientId, credit] : scheduler.clientCredits) {
        if (otherClientId == clientId) {
            continue;
        }
        credit.credit += credit.priority * request.fileSize / DEFAULT_PRIORITY /
                         FILE_CREDIT_MULTIPLIER / scheduler.numWorkers;
        if (credit.type != CreditType::IDLE) {
            credit.dequeue(scheduler);
            credit.type = CreditType::IDLE;
            credit.enqueue(scheduler);
        }
    }
    credit -= request.fileSize / FILE_CREDIT_MULTIPLIER / scheduler.numWorkers;
    if (!scheduler.requestMap[clientId].empty()) {
        if (credit > 0) {
            type = CreditType::UNDER;
            scheduler.underQueue.push_back(clientId);
            iterator = std::prev(scheduler.underQueue.end());
        } else {
            type = CreditType::OVER;
            scheduler.overQueue.push_back(clientId);
            iterator = std::prev(scheduler.overQueue.end());
        }
    } else {
        type = CreditType::IDLE;
    }
    return request;
}

std::unique_ptr<Scheduler> Scheduler::instance;
std::once_flag Scheduler::onceFlag;

/**
 * @brief Registers a new client.
 * 
 * Adds client with corresponding priority/credits to the scheduler.
 *
 * @param clientId The ID of the client to register.
 * @param priority The priority of the client.
 */
void Scheduler::registerClient(int clientId, int priority) {
    std::lock_guard<std::mutex> lock(requestMapMutex);
    clientCredits.emplace(clientId, Credit(clientId, priority, priority));
    totalCredits += priority;
}

/**
 * @brief Deregisters an existing client.
 *
 * Removes client from the scheduler.
 *
 * @param clientId The ID of the client to deregister.
 */
void Scheduler::deregisterClient(int clientId) {
    std::lock_guard<std::mutex> lock(requestMapMutex);
    auto it = clientCredits.find(clientId);
    if (it == clientCredits.end()) {
        THROW_RUNTIME_ERROR("Scheduler: clientId " + std::to_string(clientId) + " not registered");
    }
    totalCredits -= it->second.priority;
    clientCredits.erase(it);
    requestMap.erase(clientId);
}

/**
 * @brief Enqueues a client request.
 *
 * Adds a new client request to the client's request queue.
 * Wakes up workers waiting for new requests.
 *
 * @param clientId The ID of the client making the request.
 * @param MQClientRequestMessage The client request message.
 */
void Scheduler::enqueue(int clientId, MQClientRequestMessage& MQClientRequestMessage) {
    std::lock_guard<std::mutex> lock(requestMapMutex);
    requestMap[clientId].emplace_back(clientId, MQClientRequestMessage.getRequestId(),
                                      MQClientRequestMessage.getFileSize());
    requestCount++;
    auto it = clientCredits.find(clientId);
    if (it == clientCredits.end()) {
        THROW_RUNTIME_ERROR("Scheduler: clientId " + std::to_string(clientId) + " not registered");
    }
    it->second.enqueue(*this);
    requestMapCond.notify_one();
}

/**
 * @brief Schedules a client request.
 *
 * @return A unique pointer to the scheduled message handler.
 */
std::unique_ptr<IShmMessageHandler> Scheduler::schedule() {
    return schedulingStrategy->schedule(*this);
}

/**
 * @brief Schedules a client request using the round-robin strategy.
 *
 * Scans all client request queues in order to find the next request to process.
 * Stores requestMapIterator so next call can resume from the last processed request.
 *
 * @return A unique pointer to the scheduled message handler.
 */
std::unique_ptr<IShmMessageHandler> Scheduler::scheduleRoundRobin() {
    std::unique_lock<std::mutex> lock(requestMapMutex);
    requestMapCond.wait(lock, [this] { return requestCount; });

    while (requestMapIterator != requestMap.end()) {
        if (!requestMapIterator->second.empty()) {
            Request request = requestMapIterator->second.front();
            requestMapIterator->second.pop_front();
            requestMapIterator++;
            requestCount--;
            return createRoundRobinHandler(request);
        }
        requestMapIterator++;
    }
    requestMapIterator = requestMap.begin();
    while (requestMapIterator != requestMap.end()) {
        if (!requestMapIterator->second.empty()) {
            Request request = requestMapIterator->second.front();
            requestMapIterator->second.pop_front();
            requestMapIterator++;
            requestCount--;
            return createRoundRobinHandler(request);
        }
        requestMapIterator++;
    }
    THROW_RUNTIME_ERROR("FileCompressionHandler: no requests to process");
}

/**
 * @brief Schedules a client request using the credit strategy.
 * 
 * Checks top of underQueue first (clients who still have credits).
 * If no clients in underQueue, checks overQueue (clients who consumed all their credits).
 *
 * @return A unique pointer to the scheduled message handler.
 */
std::unique_ptr<IShmMessageHandler> Scheduler::scheduleCredit() {
    std::unique_lock<std::mutex> lock(requestMapMutex);
    requestMapCond.wait(lock, [this] { return requestCount; });

    if (!underQueue.empty()) {
        int clientId = underQueue.front();
        auto it = clientCredits.find(clientId);
        if (it == clientCredits.end()) {
            THROW_RUNTIME_ERROR("Scheduler: clientId " + std::to_string(clientId) +
                                " not registered");
        }
        Request request = it->second.schedule(*this);
        requestCount--;
        std::unique_ptr<IShmMessageHandler> handler = createCreditHandler(request);
        return handler;
    } else {
        int clientId = overQueue.front();
        auto it = clientCredits.find(clientId);
        if (it == clientCredits.end()) {
            THROW_RUNTIME_ERROR("Scheduler: clientId " + std::to_string(clientId) +
                                " not registered");
        }
        Request request = it->second.schedule(*this);
        requestCount--;
        std::unique_ptr<IShmMessageHandler> handler = createCreditHandler(request);
        return handler;
    }
}

/**
 * @brief Creates a message handler for a round-robin client request.
 *
 * Selects the appropriate shared memory segment and creates the handler.
 *
 * @param request The client request to process.
 * @return A unique pointer to the created message handler.
 */
std::unique_ptr<IShmMessageHandler> Scheduler::createRoundRobinHandler(Request& request) {
    // Assumes requestMapMutex is held
    return std::make_unique<ServiceFileCompressorHandler>(
        request.clientId, request.requestId, request.fileSize,
        shmManager.getShmSegment(selectRoundRobinShmSize(request)));
}

/**
 * @brief Creates a message handler for a credit client request.
 *
 * Selects the appropriate shared memory segment and creates the handler.
 *
 * @param request The client request to process.
 * @return A unique pointer to the created message handler.
 */
std::unique_ptr<IShmMessageHandler> Scheduler::createCreditHandler(Request& request) {
    // Assumes requestMapMutex is held
    return std::make_unique<ServiceFileCompressorHandler>(
        request.clientId, request.requestId, request.fileSize,
        shmManager.getShmSegment(selectCreditShmSize(request)));
}

/**
 * @brief Selects the shared memory size for a round-robin client request.
 *
 * @param request The client request to process.
 * @return The selected shared memory size.
 */
size_t Scheduler::selectRoundRobinShmSize(Request& request) {
    // Assumes requestMapMutex is held
    return std::max(shmManager.getInternalRepSize(),
                    std::min((shmManager.getSize() / (requestCount + numWorkers) +
                              shmManager.getInternalRepSize() - 1) /
                                 shmManager.getInternalRepSize() * shmManager.getInternalRepSize(),
                             (request.fileSize + shmManager.getInternalRepSize() - 1) /
                                 shmManager.getInternalRepSize() *
                                 shmManager.getInternalRepSize()));
}

/**
 * @brief Selects the shared memory size for a credit client request.
 *
 * @param request The client request to process.
 * @return The selected shared memory size.
 */
size_t Scheduler::selectCreditShmSize(Request& request) {
    // Assumes requestMapMutex is held
    return std::max(shmManager.getInternalRepSize(),
                    std::min((shmManager.getSize() / (requestCount + numWorkers) +
                              shmManager.getInternalRepSize() - 1) /
                                 shmManager.getInternalRepSize() * shmManager.getInternalRepSize(),
                             (request.fileSize + shmManager.getInternalRepSize() - 1) /
                                 shmManager.getInternalRepSize() *
                                 shmManager.getInternalRepSize()));
}
