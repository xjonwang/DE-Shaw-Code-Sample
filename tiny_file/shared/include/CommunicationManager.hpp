#pragma once

#include <string>

constexpr int REGISTRATION_MAX_MSG = 10;
constexpr int REGISTRATION_MSG_SIZE = 64;
constexpr int CLIENT_MAX_MSG = 10;
constexpr int CLIENT_MSG_SIZE = 6144;
constexpr int DEFAULT_PRIORITY = 64;
constexpr double FILE_CREDIT_MULTIPLIER = 16536.0;

class CommunicationManager {
  public:
    static CommunicationManager& getInstance() {
        static CommunicationManager instance;
        return instance;
    };
    std::string getClientToServerRegistrationQueueName();
    std::string getServerToClientRegistrationQueueName();
    std::string getClientToServerRequestQueueName(int clientId);
    std::string getServerToClientRequestQueueName(int clientId);
    std::string getShmClientSemaphoreName(int clientId, int requestId);
    std::string getShmServerSemaphoreName(int clientId, int requestId);
    std::string getShmSenseSemaphoreName(int clientId, int requestId);
    std::string getShmSegmentName(int shmId);
    CommunicationManager(CommunicationManager const&) = delete;
    void operator=(CommunicationManager const&) = delete;

  private:
    CommunicationManager(){};
};