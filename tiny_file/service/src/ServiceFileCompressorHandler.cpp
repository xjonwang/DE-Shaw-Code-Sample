#include "ServiceFileCompressorHandler.hpp"

#include "MQServerRequestMessage.hpp"
#include "ShmClientDataMessage.hpp"
#include "ShmServerDataMessage.hpp"
#include "ShmServerFileMessage.hpp"
#include "Util.hpp"
#include "snappy.h"

#include <cassert>
#include <chrono>
#include <fcntl.h>
#include <semaphore.h>
#include <stdexcept>
#include <sys/stat.h>
#include <thread>

/**
 * @brief Processes the file compression request.
 *
 * Handles the entire lifecycle of a file compression request,
 * from receiving the request to sending the response back to the client.
 */
void ServiceFileCompressorHandler::process() {
    // reset semaphores
    if (sem_unlink(communicationManager.getShmClientSemaphoreName(clientId, requestId).c_str()) ==
            -1 &&
        errno != ENOENT) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to unlink shmClientSemaphore");
    }
    sem_t* shmClientSemaphore =
        sem_open(communicationManager.getShmClientSemaphoreName(clientId, requestId).c_str(),
                 O_CREAT | O_EXCL | O_RDWR, 0666, 0);
    if (shmClientSemaphore == SEM_FAILED) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to open shmClientSemaphore");
    }
    if (sem_unlink(communicationManager.getShmServerSemaphoreName(clientId, requestId).c_str()) ==
            -1 &&
        errno != ENOENT) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to unlink shmServerSemaphore");
    }
    sem_t* shmServerSemaphore =
        sem_open(communicationManager.getShmServerSemaphoreName(clientId, requestId).c_str(),
                 O_CREAT | O_EXCL | O_RDWR, 0666, 1);
    if (shmServerSemaphore == SEM_FAILED) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to open shmServerSemaphore");
    }
    if (sem_unlink(communicationManager.getShmSenseSemaphoreName(clientId, requestId).c_str()) ==
            -1 &&
        errno != ENOENT) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to unlink shmSenseSemaphore");
    }
    sem_t* shmSenseSemaphore =
        sem_open(communicationManager.getShmSenseSemaphoreName(clientId, requestId).c_str(),
                 O_CREAT | O_EXCL | O_RDWR, 0666, 0);
    if (shmSenseSemaphore == SEM_FAILED) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to open shmSenseSemaphore");
    }

    // write message to client that signals all semaphores are ready
    MQServerRequestMessage requestMessage(requestId, shmSegment.get());
    std::unique_ptr<char[]> serializedRequestMessage = requestMessage.serialize();
    registrationVisitor.writeBlocking(clientId, serializedRequestMessage.get(),
                                      requestMessage.getContentSize());

    /**
     * communication protocol:
     * 1. client writes data to shared memory segment and notifies server by posting on shmClientSemaphore
     * 2. server processes the request by copying file contents into its own buffer
     * 3. server sends the response back to the client by posting on shmServerSemaphore
     */
    while (fileRecvSize < fileSize) {
        // wait until client data is available
        if (sem_wait(shmClientSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ServiceFileCompressorHandler: failed to wait on shmClientSemaphore");
        }
        // process client data
        processClientData();
        // signal that server has processed client data
        if (sem_post(shmServerSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ServiceFileCompressorHandler: failed to post on shmServerSemaphore");
        }
    }
    // compress data
    compressData();
    // used to decrement count to 0 so next round of transfer can proceed
    if (sem_wait(shmServerSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to wait on shmServerSemaphore");
    }
    // send post-compression file metadata
    sendFileData();
    // signal that server has sent file metadata
    if (sem_post(shmSenseSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to post on shmSenseSemaphore");
    }
    while (fileSendSize < compressedFileSize) {
        // wait for client to be ready to receive data
        if (sem_wait(shmServerSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ServiceFileCompressorHandler: failed to wait on shmServerSemaphore");
        }
        // send data
        sendClientData();
        // signal that server has sent client data
        if (sem_post(shmClientSemaphore) == -1) {
            THROW_RUNTIME_ERROR(
                "ServiceFileCompressorHandler: failed to post on shmClientSemaphore");
        }
    }

    // cleanup
    if (sem_close(shmClientSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to close shmClientSemaphore");
    }
    if (sem_close(shmServerSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to close shmServerSemaphore");
    }
    if (sem_close(shmSenseSemaphore) == -1) {
        THROW_RUNTIME_ERROR("ServiceFileCompressorHandler: failed to close shmSenseSemaphore");
    }
}

/**
 * @brief Processes the client data received via the shared memory segment.
 * 
 * Extracts client data from the shared memory segment and copies it to the
 * rawData buffer.
 */
void ServiceFileCompressorHandler::processClientData() {
    ShmClientDataMessage msg(shmSegment.get());
    msg.parse();
    msg.copyData(rawData.get() + fileRecvSize, fileSize - fileRecvSize);
    fileRecvSize += msg.getDataSize();
}

/**
 * @brief Sends data to the client via the shared memory segment.
 *
 * Writes data from the compressedData buffer to the shared memory segment.
 */
void ServiceFileCompressorHandler::sendClientData() {
    ShmServerDataMessage msg(shmSegment.get(), compressedData.data() + fileSendSize,
                             std::min(compressedFileSize - fileSendSize,
                                      shmSegment->getSize() - ShmServerDataMessage::headerSize()));
    msg.write();
    fileSendSize += msg.getDataSize();
}

/**
 * @brief Sends the post-compression file metadata to the client.
 */
void ServiceFileCompressorHandler::sendFileData() {
    ShmServerFileMessage msg(shmSegment.get(), compressedFileSize);
    msg.write();
}

/**
 * @brief Compresses the client data using Snappy compression.
 */
void ServiceFileCompressorHandler::compressData() {
    snappy::Compress(rawData.get(), fileSize, &compressedData);
    compressedFileSize = compressedData.size();
}