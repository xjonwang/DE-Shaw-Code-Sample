#include "CommunicationManager.hpp"

std::string CommunicationManager::getClientToServerRegistrationQueueName() {
    return "/tiny_file_client_registration";
}

std::string CommunicationManager::getServerToClientRegistrationQueueName() {
    return "/tiny_file_server_registration";
}

std::string CommunicationManager::getClientToServerRequestQueueName(int clientId) {
    return "/client_to_server_message_queue_" + std::to_string(clientId);
}

std::string CommunicationManager::getServerToClientRequestQueueName(int clientId) {
    return "/server_to_client_message_queue_" + std::to_string(clientId);
}

std::string CommunicationManager::getShmClientSemaphoreName(int clientId, int requestId) {
    return "/shm_client_semaphore_" + std::to_string(clientId) + "_" + std::to_string(requestId);
}

std::string CommunicationManager::getShmServerSemaphoreName(int clientId, int requestId) {
    return "/shm_server_semaphore_" + std::to_string(clientId) + "_" + std::to_string(requestId);
}

std::string CommunicationManager::getShmSenseSemaphoreName(int clientId, int requestId) {
    return "/shm_sense_semaphore_" + std::to_string(clientId) + "_" + std::to_string(requestId);
}

std::string CommunicationManager::getShmSegmentName(int shmId) {
    return "/shm_segment_" + std::to_string(shmId);
}