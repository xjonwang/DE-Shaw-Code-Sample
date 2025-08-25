#include "ClientFileCompressorWorker.hpp"
#include "MQClientRequestMessage.hpp"
#include "MQServerRequestMessage.hpp"
#include "Servicer.hpp"

ClientFileCompressorWorker::~ClientFileCompressorWorker() { stop(); }

void ClientFileCompressorWorker::run() {
    std::unique_ptr<IShmMessageHandler> handler = servicer.scheduleRequest();
    if (handler) {
        handler->process();
    }
}
