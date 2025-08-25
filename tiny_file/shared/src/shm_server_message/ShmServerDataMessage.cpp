#include "ShmServerDataMessage.hpp"
#include "Util.hpp"

#include <stdexcept>

size_t ShmServerDataMessage::headerSize() { return sizeof(dataSize); }

void ShmServerDataMessage::write() {
    if (data == nullptr) {
        THROW_RUNTIME_ERROR("ShmServerDataMessage: data is not set");
    }
    shmSegment->write(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    shmSegment->write(data, dataSize, sizeof(dataSize));
}

void ShmServerDataMessage::parse() {
    shmSegment->read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
}

size_t ShmServerDataMessage::getDataSize() { return dataSize; }

void ShmServerDataMessage::copyData(char* dest, size_t bufferSize) {
    if (bufferSize < dataSize) {
        THROW_RUNTIME_ERROR("ShmServerDataMessage: buffer size is too small");
    }
    shmSegment->read(dest, dataSize, sizeof(dataSize));
}
