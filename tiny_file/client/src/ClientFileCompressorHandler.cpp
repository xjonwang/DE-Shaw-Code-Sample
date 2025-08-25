#include "ClientFileCompressorHandler.hpp"
#include "Servicer.hpp"
#include "ShmServerFileMessage.hpp"

#include <fcntl.h>
#include <semaphore.h>
#include <stdexcept>
#include <sys/stat.h>

void ClientFileCompressorHandler::process() {
    sem_t* shmClientSemaphore = sem_open(
        communicationManager.getShmClientSemaphoreName(clientId, requestId).c_str(), O_RDWR);
    if (shmClientSemaphore == SEM_FAILED) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to open shmClientSemaphore");
    }
    sem_t* shmServerSemaphore = sem_open(
        communicationManager.getShmServerSemaphoreName(clientId, requestId).c_str(), O_RDWR);
    if (shmServerSemaphore == SEM_FAILED) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to open shmServerSemaphore");
    }
    sem_t* shmSenseSemaphore = sem_open(
        communicationManager.getShmSenseSemaphoreName(clientId, requestId).c_str(), O_RDWR);
    if (shmSenseSemaphore == SEM_FAILED) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to open shmSenseSemaphore");
    }

    while (fileSendSize < fileSize) {
        if (sem_wait(shmServerSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ClientFileCompressorHandler: failed to wait on shmServerSemaphore");
        }
        sendServerData();
        if (sem_post(shmClientSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ClientFileCompressorHandler: failed to post on shmClientSemaphore");
        }
    }
    if (sem_wait(shmSenseSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to wait on shmSenseSemaphore");
    }
    processFileData();
    
    if (sem_post(shmServerSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to post on shmServerSemaphore");
    }
    while (fileRecvSize < compressedFileSize) {
        if (sem_wait(shmClientSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ClientFileCompressorHandler: failed to wait on shmClientSemaphore");
        }
        processServerData();
        if (sem_post(shmServerSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ClientFileCompressorHandler: failed to post on shmServerSemaphore");
        }
    }

    if (sem_close(shmClientSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to close shmClientSemaphore");
    }
    if (sem_close(shmServerSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to close shmServerSemaphore");
    }
    if (sem_close(shmSenseSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to close shmSenseSemaphore");
    }
    if (sem_unlink(communicationManager.getShmClientSemaphoreName(clientId, requestId).c_str()) ==
        -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to unlink shmClientSemaphore");
    }
    if (sem_unlink(communicationManager.getShmServerSemaphoreName(clientId, requestId).c_str()) ==
        -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to unlink shmServerSemaphore");
    }
    if (sem_unlink(communicationManager.getShmSenseSemaphoreName(clientId, requestId).c_str()) ==
        -1) {
        THROW_RUNTIME_ERROR("ClientFileCompressorHandler: failed to unlink shmSenseSemaphore");
    }

    servicer.completeRequest(requestId, std::move(compressedData), compressedFileSize);
}

void ClientFileCompressorHandler::processServerData() {
    ShmServerDataMessage msg(shmSegment.get());
    msg.parse();
    msg.copyData(compressedData.get() + fileRecvSize, compressedFileSize - fileRecvSize);
    fileRecvSize += msg.getDataSize();
}

void ClientFileCompressorHandler::sendServerData() {
    ShmClientDataMessage msg(shmSegment.get(), src + fileSendSize,
                             std::min(fileSize - fileSendSize,
                                      shmSegment->getSize() - ShmClientDataMessage::headerSize()));
    msg.write();
    fileSendSize += msg.getDataSize();
}

void ClientFileCompressorHandler::processFileData() {
    ShmServerFileMessage msg(shmSegment.get());
    msg.parse();
    compressedFileSize = msg.getFileSize();
    compressedData = std::make_unique<char[]>(compressedFileSize);
}