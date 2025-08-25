#pragma once

#include "AbstractShmSegment.hpp"
#include "ClientDispatcherWorker.hpp"
#include "ClientFileCompressorHandler.hpp"
#include "ClientFileCompressorWorker.hpp"
#include "CommunicationManager.hpp"
#include "IShmMessageHandler.hpp"
#include "MQClientRequestMessage.hpp"
#include "MQServerRequestMessage.hpp"
#include "ShmManager.hpp"
#include <chrono>
#include <condition_variable>
#include <mqueue.h>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

class Servicer {
  public:
    Servicer(size_t numWorkers, int priority, const std::string& logFile);
    Servicer(size_t numWorkers, int priority) : Servicer(numWorkers, priority, ""){};
    Servicer(size_t numWorkers, const std::string& logFile)
        : Servicer(numWorkers, DEFAULT_PRIORITY, logFile){};
    Servicer(size_t numWorkers) : Servicer(numWorkers, DEFAULT_PRIORITY, ""){};
    ~Servicer();
    std::pair<std::unique_ptr<char[]>, size_t> callServiceSync(char* src, size_t size);
    int callServiceAsync(char* src, size_t size);
    std::pair<std::unique_ptr<char[]>, size_t> waitServiceSync(int requestId);

  private:
    struct Request {
        int requestId;
        size_t fileSize;
        size_t compressedSize;
        char* src;
        std::condition_variable cond;
        std::unique_ptr<char[]> compressedData;
        std::chrono::time_point<std::chrono::steady_clock> startTime;

        Request(int requestId, char* src, size_t fileSize)
            : requestId(requestId), fileSize(fileSize), src(src),
              startTime(std::chrono::steady_clock::now()) {}

        Request(Request&& other) noexcept
            : requestId(other.requestId), fileSize(other.fileSize),
              compressedSize(other.compressedSize), src(other.src),
              compressedData(std::move(other.compressedData)), startTime(other.startTime) {
            other.src = nullptr;
        }

        Request(const Request&) = delete;

        Request& operator=(Request&& other) noexcept {
            if (this != &other) {
                requestId = other.requestId;
                fileSize = other.fileSize;
                compressedSize = other.compressedSize;
                src = other.src;
                startTime = other.startTime;
                compressedData = std::move(other.compressedData);
                other.src = nullptr;
            }
            return *this;
        }
    };

    struct RequestResponse {
        int requestId;
        std::unique_ptr<AbstractShmSegment> shmSegment;
    };

    friend class ClientFileCompressorWorker;
    friend class ClientFileCompressorHandler;
    friend class ClientDispatcherWorker;

    int clientId;
    int priority;
    int nextRequestId = 0;
    std::string logFile;
    size_t numWorkers;
    CommunicationManager& communicationManager = CommunicationManager::getInstance();
    std::optional<std::reference_wrapper<ShmManager>> shmManager;

    mqd_t blockingClientToServerRegistrationQueue, blockingServerToClientRegistrationQueue;
    mqd_t blockingClientToServerRequestQueue, nonBlockingServerToClientRequestQueue;

    std::unordered_map<int, Request> requestMap;
    std::mutex requestMapMutex;

    std::queue<int> requestQueue; // queue holding unserviced requests
    size_t requestsInFlight = 0;
    std::mutex requestQueueMutex;

    std::queue<RequestResponse> requestResponseQueue; // queue holding unprocessed responses
    std::mutex requestResponseQueueMutex;
    std::condition_variable requestResponseQueueCond;

    std::unique_ptr<ClientDispatcherWorker> dispatcherWorker;
    std::vector<std::unique_ptr<ClientFileCompressorWorker>> fileCompressorWorkers;

    std::unique_ptr<IShmMessageHandler> scheduleRequest();
    std::unique_ptr<IShmMessageHandler> createHandler(RequestResponse& requestResponse);
    void completeRequest(int requestId, std::unique_ptr<char[]> compressedData,
                         size_t compressedSize);
    void sendRequests();
    void receiveResponses();
};