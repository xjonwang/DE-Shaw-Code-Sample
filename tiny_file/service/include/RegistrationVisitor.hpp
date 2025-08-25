#pragma once

#include <mqueue.h>
#include <unordered_map>
#include <unordered_set>

#include "CommunicationManager.hpp"
#include "MQClientQueueCreationMessage.hpp"
#include "MQClientQueueDeletionMessage.hpp"
#include "MQClientRegistrationMessage.hpp"
#include "Scheduler.hpp"
#include "ShmManager.hpp"

/**
 * @class RegistrationVisitor
 * @brief Handles the registration and management of client message queues.
 * 
 * Processes registration messages using Visitor pattern.
 * Registers new clients, opens their message queues, and manages their lifecycle.
 */
class RegistrationVisitor {
  public:
    static RegistrationVisitor& getInstance() {
        static RegistrationVisitor instance;
        return instance;
    };
    void visitRegistration(MQClientRegistrationMessage&);
    void visitQueueCreation(MQClientQueueCreationMessage&);
    void visitQueueDeletion(MQClientQueueDeletionMessage&);
    void dispatch();
    void reg();
    void writeBlocking(int, char*, size_t);

    RegistrationVisitor(const RegistrationVisitor&) = delete;
    RegistrationVisitor& operator=(const RegistrationVisitor&) = delete;

  private:
    RegistrationVisitor();
    int nextClientId = 0;
    std::unordered_set<int> registeredClients;
    std::unordered_map<int, mqd_t> blockingServerToClientQueues, nonBlockingClientToServerQueues;
    mqd_t nonBlockingClientToServerQueue, blockingServerToClientQueue;
    Scheduler& scheduler = Scheduler::getInstance();
    CommunicationManager& communicationManager = CommunicationManager::getInstance();
    ShmManager& shmManager = ShmManager::getInstance();
};