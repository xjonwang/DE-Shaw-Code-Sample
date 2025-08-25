#include "ClientDispatcherWorker.hpp"
#include "Servicer.hpp"

ClientDispatcherWorker::~ClientDispatcherWorker() { stop(); }

void ClientDispatcherWorker::run() {
    servicer.sendRequests();
    servicer.receiveResponses();
}