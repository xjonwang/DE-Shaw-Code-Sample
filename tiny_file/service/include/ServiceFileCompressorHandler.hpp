#pragma once

#include "AbstractShmSegment.hpp"
#include "CommunicationManager.hpp"
#include "IShmMessageHandler.hpp"
#include "RegistrationVisitor.hpp"
#include <memory>

/**
 * @class ServiceFileCompressorHandler
 * @brief Handles the compression of files for a specific client request.
 *
 * Implements the IShmMessageHandler interface to process file compression requests.
 * Manages shared memory segments and semaphores for communication with the client.
 *
 * @param clientId The ID of the client making the request.
 * @param requestId The ID of the request.
 * @param fileSize The size of the file being processed.
 * @param shmSegment The shared memory segment used for communication.
 */
class ServiceFileCompressorHandler : public IShmMessageHandler {
  public:
    ServiceFileCompressorHandler(int clientId, int requestId, size_t fileSize,
                                 std::unique_ptr<AbstractShmSegment> shmSegment)
        : clientId(clientId), requestId(requestId), fileSize(fileSize),
          shmSegment(std::move(shmSegment)), rawData(std::make_unique<char[]>(fileSize)){};
    void process();

  private:
    int clientId;
    int requestId;
    size_t fileSize;
    size_t compressedFileSize;
    size_t fileRecvSize = 0;
    size_t fileSendSize = 0;
    std::unique_ptr<AbstractShmSegment> shmSegment;
    std::unique_ptr<char[]> rawData;
    std::string compressedData;
    CommunicationManager& communicationManager = CommunicationManager::getInstance();
    RegistrationVisitor& registrationVisitor = RegistrationVisitor::getInstance();
    void compressData();
    void processClientData();
    void sendClientData();
    void sendFileData();
};