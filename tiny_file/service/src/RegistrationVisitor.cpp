#include "RegistrationVisitor.hpp"

#include "AbstractMQClientRegistrationMessage.hpp"
#include "MQServerIdMessage.hpp"
#include <stdexcept>

/**
 * @brief Constructs a RegistrationVisitor.
 *
 * Initializes global registration queues for client-server + server-client communication.
 */
RegistrationVisitor::RegistrationVisitor() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = REGISTRATION_MAX_MSG;
    attr.mq_msgsize = REGISTRATION_MSG_SIZE;
    attr.mq_curmsgs = 0;
    mq_unlink(communicationManager.getClientToServerRegistrationQueueName().c_str());
    nonBlockingClientToServerQueue =
        mq_open(communicationManager.getClientToServerRegistrationQueueName().c_str(),
                O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK, 0666, &attr);
    if (nonBlockingClientToServerQueue == -1) {
        THROW_RUNTIME_ERROR(
            "RegistrationVisitor: failed to open non-blocking C2S message queue with errno: " +
            std::to_string(errno));
    }
    mq_unlink(communicationManager.getServerToClientRegistrationQueueName().c_str());
    blockingServerToClientQueue =
        mq_open(communicationManager.getServerToClientRegistrationQueueName().c_str(),
                O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
    if (blockingServerToClientQueue == -1) {
        THROW_RUNTIME_ERROR(
            "RegistrationVisitor: failed to create blocking S2C message queue with errno: " +
            std::to_string(errno));
    }
}

/**
 * @brief Handles the registration of a new client.
 *
 * Assigns a new client ID and sends it back to the client along with shared memory parameters.
 *
 * @param message The registration message.
 */
void RegistrationVisitor::visitRegistration(MQClientRegistrationMessage& /*message*/) {
    MQServerIdMessage idMessage(nextClientId, shmManager.getNumShmSegments(),
                                shmManager.getShmSegmentSize(), shmManager.getInternalRepSize());
    std::unique_ptr<char[]> msg = idMessage.serialize();
    if (mq_send(blockingServerToClientQueue, msg.get(), idMessage.getContentSize(), 0) == -1) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: failed to send message");
    }
    nextClientId++;
}

/**
 * @brief Handles the creation of a client queue.
 *
 * Adds clientId to the list of registered clients.
 * Adds client created message queues to map.
 *
 * @param message The queue creation message.
 */
void RegistrationVisitor::visitQueueCreation(MQClientQueueCreationMessage& message) {
    int clientId = message.getClientId();
    if (registeredClients.find(clientId) != registeredClients.end()) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: clientId " + std::to_string(clientId) +
                            " already registered");
    }
    registeredClients.insert(clientId);
    scheduler.registerClient(clientId, message.getPriority());
    blockingServerToClientQueues[clientId] =
        mq_open(communicationManager.getServerToClientRequestQueueName(clientId).c_str(), O_RDWR);
    if (blockingServerToClientQueues[clientId] == -1) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: failed to open message queue");
    }
    nonBlockingClientToServerQueues[clientId] =
        mq_open(communicationManager.getClientToServerRequestQueueName(clientId).c_str(),
                O_RDWR | O_NONBLOCK);
    if (nonBlockingClientToServerQueues[clientId] == -1) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: failed to open message queue");
    }
}

/**
 * @brief Handles the deletion of a client queue.
 *
 * Removes clientId from the list of registered clients.
 * Closes client created message queues.
 *
 * @param message The queue deletion message.
 */
void RegistrationVisitor::visitQueueDeletion(MQClientQueueDeletionMessage& message) {
    int clientId = message.getClientId();
    if (registeredClients.find(clientId) == registeredClients.end()) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: clientId " + std::to_string(clientId) +
                            " not registered");
    }
    if (mq_close(blockingServerToClientQueues[clientId]) == -1) {
        THROW_RUNTIME_ERROR(
            "RegistrationVisitor: failed to close blockingServerToClientQueue for " +
            std::to_string(clientId) + " with errno: " + std::to_string(errno));
    }
    if (mq_close(nonBlockingClientToServerQueues[clientId]) == -1) {
        THROW_RUNTIME_ERROR(
            "RegistrationVisitor: failed to close nonBlockingClientToServerQueue for " +
            std::to_string(clientId) + " with errno: " + std::to_string(errno));
    }
    registeredClients.erase(clientId);
    scheduler.deregisterClient(clientId);
    blockingServerToClientQueues.erase(clientId);
    nonBlockingClientToServerQueues.erase(clientId);
}

/**
 * @brief Dispatches incoming client requests.
 *
 * Scans every client-to-server queue (non-blocking) for incoming messages and queues them onto the scheduler runqueue.
 */
void RegistrationVisitor::dispatch() {
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(CLIENT_MSG_SIZE);
    for (auto& [clientId, mq] : nonBlockingClientToServerQueues) {
        while (mq_receive(mq, buffer.get(), CLIENT_MSG_SIZE, nullptr) != -1) {
            MQClientRequestMessage message = MQClientRequestMessage::deserialize(buffer.get());
            scheduler.enqueue(clientId, message);
        }
    }
}

/**
 * @brief Registers new clients.
 *
 * Scans global registration queue for incoming client registration messages and processes them.
 * Deserializes and demultiplexes messages, then calls accept to visit.
 */
void RegistrationVisitor::reg() {
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(REGISTRATION_MSG_SIZE);
    while (mq_receive(nonBlockingClientToServerQueue, buffer.get(), REGISTRATION_MSG_SIZE,
                      nullptr) != -1) {
        std::unique_ptr<AbstractMQClientRegistrationMessage> message =
            AbstractMQClientRegistrationMessage::deserialize(buffer.get());
        message->accept(*this);
    }
}

/**
 * @brief Writes a message to server-to-client queue (blocking).
 *
 * @param clientId The ID of the client.
 * @param src The source buffer containing the message.
 * @param size The size of the message.
 */
void RegistrationVisitor::writeBlocking(int clientId, char* src, size_t size) {
    if (size > CLIENT_MSG_SIZE) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: message too large");
    }
    if (registeredClients.find(clientId) == registeredClients.end()) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: clientId " + std::to_string(clientId) +
                            " not registered");
    }
    if (mq_send(blockingServerToClientQueues[clientId], src, size, 0) == -1) {
        THROW_RUNTIME_ERROR("RegistrationVisitor: failed to send message");
    }
}