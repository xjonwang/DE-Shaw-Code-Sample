#include "ShmServerFileMessage.hpp"

#include <stdexcept>

void ShmServerFileMessage::write() {
    shmSegment->write(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
}

void ShmServerFileMessage::parse() {
    shmSegment->read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
}

size_t ShmServerFileMessage::getFileSize() { return fileSize; }