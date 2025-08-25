#pragma once

#include "AbstractShmSegment.hpp"
#include "CommunicationManager.hpp"
#include "IShmMessageHandler.hpp"
#include "ShmClientDataMessage.hpp"
#include "ShmServerDataMessage.hpp"
#include <memory>

class Servicer;

class ClientFileCompressorHandler : public IShmMessageHandler {
  public:
    ClientFileCompressorHandler(int clientId, int requestId, char* src, size_t fileSize,
                                std::unique_ptr<AbstractShmSegment> shmSegment, Servicer& servicer)
        : clientId(clientId), requestId(requestId), servicer(servicer), src(src),
          fileSize(fileSize), shmSegment(std::move(shmSegment)){};
    void process() override;

  private:
    int clientId;
    int requestId;
    Servicer& servicer;
    char* src;
    size_t fileSize;
    size_t compressedFileSize;
    size_t fileSendSize = 0;
    size_t fileRecvSize = 0;
    std::unique_ptr<AbstractShmSegment> shmSegment;
    std::unique_ptr<char[]> compressedData;
    CommunicationManager& communicationManager = CommunicationManager::getInstance();
    void processServerData();
    void sendServerData();
    void processFileData();
};