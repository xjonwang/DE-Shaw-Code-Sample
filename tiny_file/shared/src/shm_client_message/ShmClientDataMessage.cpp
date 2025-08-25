#include "ShmClientDataMessage.hpp"
#include "Util.hpp"

#include <stdexcept>

size_t ShmClientDataMessage::headerSize() { return sizeof(dataSize); }

void ShmClientDataMessage::write() {
    if (data == nullptr) {
        THROW_RUNTIME_ERROR("ShmClientDataMessage: data is not set");
    }
    shmSegment->write(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    shmSegment->write(data, dataSize, sizeof(dataSize));
}

void ShmClientDataMessage::parse() {
    shmSegment->read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
}

size_t ShmClientDataMessage::getDataSize() { return dataSize; }

void ShmClientDataMessage::copyData(char* dest, size_t bufferSize) {
    if (bufferSize < dataSize) {
        THROW_RUNTIME_ERROR("ShmClientDataMessage: buffer size is too small");
    }
    shmSegment->read(dest, dataSize, sizeof(dataSize));
}
