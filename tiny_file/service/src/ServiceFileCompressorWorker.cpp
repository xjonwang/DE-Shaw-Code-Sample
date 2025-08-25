#include "ServiceFileCompressorWorker.hpp"

/**
 * @brief Runs the service file compressor worker.
 *
 * Continuously asks scheduler for next request and processes it.
 */
void ServiceFileCompressorWorker::run() {
    while (true) {
        std::unique_ptr<IShmMessageHandler> handler = scheduler.schedule();
        handler->process();
    }
}