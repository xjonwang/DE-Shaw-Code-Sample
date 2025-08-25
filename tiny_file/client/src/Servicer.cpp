#include "Servicer.hpp"
#include "ClientDispatcherWorker.hpp"
#include "ClientFileCompressorWorker.hpp"
#include "MQClientQueueCreationMessage.hpp"
#include "MQClientQueueDeletionMessage.hpp"
#include "MQClientRegistrationMessage.hpp"
#include "MQServerIdMessage.hpp"

#include <chrono>
#include <fstream>
#include <iostream>

Servicer::Servicer(size_t numWorkers, int priority, const std::string& logFile)
    : priority(priority), logFile(logFile), numWorkers(numWorkers) {
    int status;
    blockingClientToServerRegistrationQueue =
        mq_open(communicationManager.getClientToServerRegistrationQueueName().c_str(), O_RDWR);
    if (blockingClientToServerRegistrationQueue == -1) {
        THROW_RUNTIME_ERROR("Servicer: failed to open blockingClientToServerRegistrationQueue");
    }
    blockingServerToClientRegistrationQueue =
        mq_open(communicationManager.getServerToClientRegistrationQueueName().c_str(), O_RDWR);
    if (blockingServerToClientRegistrationQueue == -1) {
        THROW_RUNTIME_ERROR("Servicer: failed to open blockingServerToClientRegistrationQueue");
    }

    // Register client with server to get unique client id
    MQClientRegistrationMessage registrationMessage;
    std::unique_ptr<char[]> sendRegistrationMessageBuffer = registrationMessage.serialize();
    status = mq_send(blockingClientToServerRegistrationQueue, sendRegistrationMessageBuffer.get(),
                     registrationMessage.getContentSize(), 0);
    if (status == -1) {
        THROW_RUNTIME_ERROR("Servicer: failed to send registration message");
    }

    // Wait for server to respond with client id
    std::unique_ptr<char[]> recvRegistrationMessageBuffer =
        std::make_unique<char[]>(REGISTRATION_MSG_SIZE);
    status = mq_receive(blockingServerToClientRegistrationQueue,
                        recvRegistrationMessageBuffer.get(), REGISTRATION_MSG_SIZE, nullptr);
    if (status == -1) {
        THROW_RUNTIME_ERROR("Servicer: failed to receive registration message");
    }
    MQServerIdMessage serverIdMessage =
        MQServerIdMessage::deserialize(recvRegistrationMessageBuffer.get());
    clientId = serverIdMessage.getClientId();
    ShmManager::init(serverIdMessage.getNumShmSegments(), serverIdMessage.getShmSegmentSize(),
                     serverIdMessage.getInternalRepSize());
    shmManager = ShmManager::getInstance();

    // Create client queue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = CLIENT_MAX_MSG;
    attr.mq_msgsize = CLIENT_MSG_SIZE;
    attr.mq_curmsgs = 0;
    mq_unlink(communicationManager.getClientToServerRequestQueueName(clientId).c_str());
    blockingClientToServerRequestQueue =
        mq_open(communicationManager.getClientToServerRequestQueueName(clientId).c_str(),
                O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
    if (blockingClientToServerRequestQueue == -1) {
        THROW_RUNTIME_ERROR(
            "Servicer: failed to open blockingClientToServerRequestQueue with errno: " +
            std::to_string(errno));
    }
    mq_unlink(communicationManager.getServerToClientRequestQueueName(clientId).c_str());
    nonBlockingServerToClientRequestQueue =
        mq_open(communicationManager.getServerToClientRequestQueueName(clientId).c_str(),
                O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK, 0666, &attr);
    if (nonBlockingServerToClientRequestQueue == -1) {
        THROW_RUNTIME_ERROR(
            "Servicer: failed to open nonBlockingServerToClientRequestQueue with errno: " +
            std::to_string(errno));
    }
    MQClientQueueCreationMessage queueCreationMessage(clientId, priority);
    std::unique_ptr<char[]> queueCreationMessageBuffer = queueCreationMessage.serialize();
    status = mq_send(blockingClientToServerRegistrationQueue, queueCreationMessageBuffer.get(),
                     queueCreationMessage.getContentSize(), 0);
    if (status == -1) {
        THROW_RUNTIME_ERROR("Servicer: failed to send queue creation message");
    }
    dispatcherWorker = std::make_unique<ClientDispatcherWorker>(*this);
    fileCompressorWorkers.reserve(numWorkers);
    for (size_t i = 0; i < numWorkers; i++) {
        fileCompressorWorkers.push_back(std::make_unique<ClientFileCompressorWorker>(i, *this));
    }
    dispatcherWorker->start();
    for (auto& fileCompressorWorker : fileCompressorWorkers) {
        fileCompressorWorker->start();
    }
}

Servicer::~Servicer() {
    // Notify server of client queue deletion
    MQClientQueueDeletionMessage queueDeletionMessage(clientId);
    std::unique_ptr<char[]> queueDeletionMessageBuffer = queueDeletionMessage.serialize();
    mq_send(blockingClientToServerRegistrationQueue, queueDeletionMessageBuffer.get(),
            queueDeletionMessage.getContentSize(), 0);

    // Perform client side queue cleanup
    mq_close(blockingClientToServerRegistrationQueue);
    mq_close(blockingServerToClientRegistrationQueue);
    mq_close(blockingClientToServerRequestQueue);
    mq_close(nonBlockingServerToClientRequestQueue);
    mq_unlink(communicationManager.getClientToServerRequestQueueName(clientId).c_str());
    mq_unlink(communicationManager.getServerToClientRequestQueueName(clientId).c_str());
}

std::pair<std::unique_ptr<char[]>, size_t> Servicer::callServiceSync(char* src, size_t size) {
    std::unique_lock<std::mutex> queueLock(requestQueueMutex);
    std::unique_lock<std::mutex> mapLock(requestMapMutex);
    int tempNextRequestId = nextRequestId;
    requestMap.emplace(tempNextRequestId, Request(nextRequestId, src, size));
    requestQueue.push(tempNextRequestId);
    nextRequestId++;

    queueLock.unlock();
    auto it = requestMap.find(tempNextRequestId);
    if (it == requestMap.end()) {
        THROW_RUNTIME_ERROR("Servicer: request not found");
    }
    it->second.cond.wait(mapLock, [this, tempNextRequestId] {
        auto it = requestMap.find(tempNextRequestId);
        if (it == requestMap.end()) {
            THROW_RUNTIME_ERROR("Servicer: request not found");
        }
        return it->second.compressedData != nullptr;
    });
    it = requestMap.find(tempNextRequestId);
    if (it == requestMap.end()) {
        THROW_RUNTIME_ERROR("Servicer: request not found");
    }
    std::pair<std::unique_ptr<char[]>, size_t> ret =
        std::make_pair(std::move(it->second.compressedData), it->second.compressedSize);
    int64_t cst = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now() - it->second.startTime)
                      .count();
    std::cout << "Servicer: request " << clientId << "," << it->second.requestId << " took " << cst
              << " us" << std::endl;
    if (logFile != "") {
        std::ofstream log(logFile, std::ios::app);
        log << cst << std::endl;
    }
    requestMap.erase(it);
    return ret;
}

int Servicer::callServiceAsync(char* src, size_t size) {
    std::lock_guard<std::mutex> queueLock(requestQueueMutex);
    std::lock_guard<std::mutex> mapLock(requestMapMutex);
    requestMap.emplace(nextRequestId, Request(nextRequestId, src, size));
    requestQueue.push(nextRequestId);
    return nextRequestId++;
}

std::pair<std::unique_ptr<char[]>, size_t> Servicer::waitServiceSync(int requestId) {
    std::unique_lock<std::mutex> lock(requestMapMutex);
    auto it = requestMap.find(requestId);
    if (it == requestMap.end()) {
        THROW_RUNTIME_ERROR("Servicer: request not found");
    }
    it->second.cond.wait(lock, [this, requestId] {
        auto it = requestMap.find(requestId);
        if (it == requestMap.end()) {
            THROW_RUNTIME_ERROR("Servicer: request not found");
        }
        return it->second.compressedData != nullptr;
    });
    it = requestMap.find(requestId);
    if (it == requestMap.end()) {
        THROW_RUNTIME_ERROR("Servicer: request not found");
    }
    std::pair<std::unique_ptr<char[]>, size_t> ret =
        std::make_pair(std::move(it->second.compressedData), it->second.compressedSize);
    std::cout << "Servicer: request " << clientId << "," << it->second.requestId << " took "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::steady_clock::now() - it->second.startTime)
                     .count()
              << " us" << std::endl;
    if (logFile != "") {
        std::ofstream log(logFile, std::ios::app);
        log << std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now() - it->second.startTime)
                   .count()
            << std::endl;
    }
    requestMap.erase(it);
    return ret;
}

std::unique_ptr<IShmMessageHandler> Servicer::scheduleRequest() {
    std::unique_lock<std::mutex> lock(requestResponseQueueMutex);
    if (requestResponseQueueCond.wait_for(lock, std::chrono::seconds(1),
                                          [this] { return !requestResponseQueue.empty(); })) {
        std::unique_ptr<IShmMessageHandler> handler = createHandler(requestResponseQueue.front());
        requestResponseQueue.pop();
        return handler;
    } else {
        return nullptr;
    }
}

void Servicer::completeRequest(int requestId, std::unique_ptr<char[]> compressedData,
                               size_t compressedSize) {
    std::lock_guard<std::mutex> queueLock(requestQueueMutex);
    std::lock_guard<std::mutex> mapLock(requestMapMutex);
    requestsInFlight--;
    auto it = requestMap.find(requestId);
    if (it == requestMap.end()) {
        THROW_RUNTIME_ERROR("Servicer: request not found");
    }
    Request& request = it->second;
    request.compressedData = std::move(compressedData);
    request.compressedSize = compressedSize;
    request.cond.notify_one();
}

std::unique_ptr<IShmMessageHandler> Servicer::createHandler(RequestResponse& requestResponse) {
    std::lock_guard<std::mutex> lock(requestMapMutex);
    auto it = requestMap.find(requestResponse.requestId);
    if (it == requestMap.end()) {
        THROW_RUNTIME_ERROR("Servicer: request not found");
    }
    return std::make_unique<ClientFileCompressorHandler>(
        clientId, requestResponse.requestId, it->second.src, it->second.fileSize,
        std::move(requestResponse.shmSegment), *this);
}

void Servicer::sendRequests() {
    std::unique_lock<std::mutex> queueLock(requestQueueMutex);
    std::unique_lock<std::mutex> mapLock(requestMapMutex);
    while (requestsInFlight < numWorkers && !requestQueue.empty()) {
        int id = requestQueue.front();
        requestQueue.pop();
        auto it = requestMap.find(id);
        if (it == requestMap.end()) {
            THROW_RUNTIME_ERROR("Servicer: request not found");
        }
        Request& request = it->second;
        requestsInFlight++;

        queueLock.unlock();
        mapLock.unlock();
        MQClientRequestMessage requestMessage(request.requestId, request.fileSize);
        std::unique_ptr<char[]> requestMessageBuffer = requestMessage.serialize();
        if (mq_send(blockingClientToServerRequestQueue, requestMessageBuffer.get(),
                    requestMessage.getContentSize(), 0) == -1) {
            THROW_RUNTIME_ERROR("Servicer: failed to send request message");
        }
        queueLock.lock();
        mapLock.lock();
    }
}

void Servicer::receiveResponses() {
    std::unique_ptr<char[]> responseMessageBuffer = std::make_unique<char[]>(CLIENT_MSG_SIZE);
    while (mq_receive(nonBlockingServerToClientRequestQueue, responseMessageBuffer.get(),
                      CLIENT_MSG_SIZE, nullptr) != -1) {
        std::lock_guard<std::mutex> lock(requestResponseQueueMutex);
        MQServerRequestMessage responseMessage =
            MQServerRequestMessage::deserialize(responseMessageBuffer.get());
        RequestResponse requestResponse;
        requestResponse.requestId = responseMessage.getRequestId();
        requestResponse.shmSegment =
            shmManager->get().getShmSegment(responseMessage.getShmSegmentIds());
        requestResponseQueue.push(std::move(requestResponse));
        requestResponseQueueCond.notify_one();
    }
}