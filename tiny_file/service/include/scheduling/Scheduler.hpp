#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "CreditSchedulingStrategy.hpp"
#include "ISchedulingStrategy.hpp"
#include "IShmMessageHandler.hpp"
#include "MQClientRequestMessage.hpp"
#include "RoundRobinSchedulingStrategy.hpp"
#include "ShmManager.hpp"

/**
 * @class Scheduler
 * @brief Singleton for managing scheduling of client requests
 *
 * Uses a scheduling strategy (credit, round-robin) to determine the order in which client requests are processed.
 * Tracks client registrations and client requests. Scheduling works by maintaining a queue of requests for each client and creating a
 * message handler based off the request to process it. This request handler gets passed to a worker thread that simply calls the process() method.
 *
 * @param numWorkers Number of worker threads available
 * @param schedulingStrategy Scheduling strategy to use
 */
class Scheduler {
  public:
    static void init(size_t numWorkers, std::unique_ptr<ISchedulingStrategy> schedulingStrategy) {
        std::call_once(onceFlag,
                       [numWorkers, schedulingStrategy = std::move(schedulingStrategy)]() mutable {
                           instance.reset(new Scheduler(numWorkers, std::move(schedulingStrategy)));
                       });
    };
    static Scheduler& getInstance() {
        if (!instance) {
            THROW_RUNTIME_ERROR("Scheduler: instance not initialized");
        }
        return *instance;
    };
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void registerClient(int, int);
    void deregisterClient(int);
    void enqueue(int, MQClientRequestMessage&);
    std::unique_ptr<IShmMessageHandler> schedule();

  private:
    /**
     * @struct Request
     * @brief Represents a client request.
     *
     * Contains the necessary information to process a client's request.
     *
     * @param clientId The ID of the client making the request.
     * @param requestId The ID of the request.
     * @param fileSize The size of the file being processed.
     */
    struct Request {
        int clientId;
        int requestId;
        size_t fileSize;

        Request(int clientId, int requestId, size_t fileSize)
            : clientId(clientId), requestId(requestId), fileSize(fileSize){};
    };

    enum class CreditType {
        UNDER,
        OVER,
        IDLE,
    };

    /**
     * @struct Credit
     * @brief Represents credit scheduling state of a client.
     * 
     * Keeps track of client's priority, current credit, and type (UNDER, OVER, IDLE).
     * Moves clients between underQueue and overQueue based on their credit status.
     * 
     * @param clientId The ID of the client.
     * @param priority The priority of the client.
     * @param credit The initial credit of the client.
     */
    struct Credit {
        int clientId;
        double priority;
        double credit;
        CreditType type;
        std::list<int>::iterator iterator;

        Credit(int clientId, int priority, double credit) : clientId(clientId), priority(priority), credit(credit), type(CreditType::IDLE) {}
        void enqueue(Scheduler& scheduler);
        void dequeue(Scheduler& scheduler);
        Request schedule(Scheduler& scheduler);
    };

    friend struct Credit;

    Scheduler(size_t numWorkers, std::unique_ptr<ISchedulingStrategy> schedulingStrategy)
        : numWorkers(numWorkers), schedulingStrategy(std::move(schedulingStrategy)){};
    static std::unique_ptr<Scheduler> instance;
    static std::once_flag onceFlag;

    size_t numWorkers;
    std::unique_ptr<ISchedulingStrategy> schedulingStrategy;

    int totalCredits = 0;
    std::unordered_map<int, Credit> clientCredits;          // clientId to priority
    std::list<int> underQueue, overQueue;                   // clientIds
    std::unordered_map<int, std::list<Request>> requestMap; // clientId to list of requests
    std::unordered_map<int, std::list<Request>>::iterator
        requestMapIterator; // iterator for requestMap
    size_t requestCount = 0;
    std::mutex requestMapMutex;
    std::condition_variable requestMapCond;
    ShmManager& shmManager = ShmManager::getInstance();

    size_t selectRoundRobinShmSize(Request&);
    size_t selectCreditShmSize(Request&);
    std::unique_ptr<IShmMessageHandler> createRoundRobinHandler(Request&);
    std::unique_ptr<IShmMessageHandler> createCreditHandler(Request&);
    std::unique_ptr<IShmMessageHandler> scheduleRoundRobin();
    std::unique_ptr<IShmMessageHandler> scheduleCredit();

    friend class RoundRobinSchedulingStrategy;
    friend class CreditSchedulingStrategy;
};