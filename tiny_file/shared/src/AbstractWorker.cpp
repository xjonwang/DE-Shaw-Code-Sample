#include "AbstractWorker.hpp"

void AbstractWorker::start() {
    if (running)
        return;
    running = true;
    workerThread = std::thread(&AbstractWorker::threadLoop, this);
}

void AbstractWorker::stop() {
    if (!running)
        return;
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void AbstractWorker::threadLoop() {
    while (running) {
        run();
    }
}