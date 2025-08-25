#include "ArgParse.hpp"
#include "Servicer.hpp"
#include "snappy.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

enum class ProcessState { SYNC, ASYNC };

std::unique_ptr<char[]> readFile(const std::string& filePath, size_t& fileSize) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        THROW_RUNTIME_ERROR("Failed to open file: " + filePath);
    }

    fileSize = std::filesystem::file_size(filePath);

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(fileSize);
    if (!file.read(buffer.get(), fileSize)) {
        THROW_RUNTIME_ERROR("Failed to read file: " + filePath);
    }

    return buffer;
}

void processFileSync(const std::string& file, Servicer& servicer) {
    size_t fileSize;
    std::unique_ptr<char[]> fileData = readFile(file, fileSize);
    auto [compressedData, compressedSize] = servicer.callServiceSync(fileData.get(), fileSize);
    std::string uncompressedFileData;
    snappy::Uncompress(compressedData.get(), compressedSize, &uncompressedFileData);
    if (uncompressedFileData != std::string(fileData.get(), fileSize)) {
        THROW_RUNTIME_ERROR("Uncompressed data does not match original data");
    } else {
        std::cout << GREEN << "SUCCESS: Uncompressed data matches original data" << RESET
                  << std::endl;
    }
}

void processFileAsync(const std::string& file, Servicer& servicer) {
    size_t fileSize;
    std::unique_ptr<char[]> fileData = readFile(file, fileSize);
    std::cout << YELLOW << "Sending Request: " << file << RESET << std::endl;
    int requestId = servicer.callServiceAsync(fileData.get(), fileSize);
    std::cout << YELLOW << "Request ID: " << requestId << RESET << std::endl;
    std::cout << YELLOW << "Waiting for response..." << RESET << std::endl;
    auto [compressedData, compressedSize] = servicer.waitServiceSync(requestId);
    std::string uncompressedFileData;
    snappy::Uncompress(compressedData.get(), compressedSize, &uncompressedFileData);
    if (uncompressedFileData != std::string(fileData.get(), fileSize)) {
        THROW_RUNTIME_ERROR("Uncompressed data does not match original data");
    } else {
        std::cout << GREEN << "SUCCESS: Uncompressed data matches original data" << RESET
                  << std::endl;
    }
}

void processFileAsync(const std::vector<std::string>& files, Servicer& servicer) {
    size_t numFiles = files.size();
    std::vector<size_t> fileSizes(numFiles);
    std::vector<std::unique_ptr<char[]>> fileData(numFiles);
    std::vector<int> requestIds(numFiles);
    for (size_t i = 0; i < numFiles; i++) {
        fileData[i] = readFile(files[i], fileSizes[i]);
        std::cout << YELLOW << "Sending Request: " << files[i] << RESET << std::endl;
        requestIds[i] = servicer.callServiceAsync(fileData[i].get(), fileSizes[i]);
        std::cout << YELLOW << "Request ID: " << requestIds[i] << RESET << std::endl;
    }
    for (size_t i = 0; i < numFiles; i++) {
        std::cout << YELLOW << "Waiting for response..." << RESET << std::endl;
        auto [compressedData, compressedSize] = servicer.waitServiceSync(requestIds[i]);
        std::string uncompressedFileData;
        snappy::Uncompress(compressedData.get(), compressedSize, &uncompressedFileData);
        if (uncompressedFileData != std::string(fileData[i].get(), fileSizes[i])) {
            THROW_RUNTIME_ERROR("Uncompressed data does not match original data");
        } else {
            std::cout << GREEN << "SUCCESS: Uncompressed data matches original data" << RESET
                      << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    argparse::ArgumentParser program("app");

    program.add_argument("--state")
        .required()
        .help("Set the operation state to SYNC or ASYNC")
        .action([](const std::string& value) {
            if (value == "SYNC") {
                std::cout << "State: SYNC" << std::endl;
                return ProcessState::SYNC;
            } else if (value == "ASYNC") {
                std::cout << "State: ASYNC" << std::endl;
                return ProcessState::ASYNC;
            } else {
                THROW_RUNTIME_ERROR("Invalid state: " + value);
            }
        });

    program.add_argument("--file").help("Specify the file path to be compressed");

    program.add_argument("--files").help(
        "Specify the file containing the list of files to be compressed (one file per line)");

    program.add_argument("--priority").scan<'i', int>().help("Specify the credit priority");

    program.add_argument("--log").help("Specify the log file path");

    ProcessState state;
    std::string filePath;
    bool fileFlag = false;
    std::string fileListPath;
    bool filesFlag = false;
    int priority = DEFAULT_PRIORITY;
    std::string logPath = "";
    try {
        program.parse_args(argc, argv);

        state = program.get<ProcessState>("--state");

        if (program.is_used("--file")) {
            filePath = program.get<std::string>("--file");
            fileFlag = true;
            std::cout << "File to compress: " << filePath << std::endl;
        }
        if (program.is_used("--files")) {
            fileListPath = program.get<std::string>("--files");
            filesFlag = true;
            std::cout << "File list to compress: " << fileListPath << std::endl;
        }
        if (!fileFlag && !filesFlag) {
            std::cerr << "Error: Please provide either --file or --files option" << std::endl;
            return 1;
        }

        if (program.is_used("--priority")) {
            priority = program.get<int>("--priority");
            std::cout << "Priority: " << priority << std::endl;
        }

        if (program.is_used("--log")) {
            logPath = program.get<std::string>("--log");
            std::cout << "Log file: " << logPath << std::endl;
        }
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return 1;
    }

    Servicer servicer(4, 64, "");

    switch (state) {
    case ProcessState::SYNC:
        if (fileFlag) {
            processFileSync(filePath, servicer);
        }
        if (filesFlag) {
            std::ifstream fileList(fileListPath);
            if (!fileList.is_open()) {
                THROW_RUNTIME_ERROR("Failed to open file: " + fileListPath);
            }
            while (std::getline(fileList, filePath)) {
                processFileSync(filePath, servicer);
            }
        }
        break;
    case ProcessState::ASYNC:
        if (fileFlag) {
            processFileAsync(filePath, servicer);
        }
        if (filesFlag) {
            std::ifstream fileList(fileListPath);
            if (!fileList.is_open()) {
                THROW_RUNTIME_ERROR("Failed to open file: " + fileListPath);
            }
            std::vector<std::string> files;
            while (std::getline(fileList, filePath)) {
                files.push_back(filePath);
            }
            processFileAsync(files, servicer);
        }
        break;
    }
    return 0;
}