#pragma once

#include <atomic>
#include <thread>

class AbstractWorker {
  public:
    AbstractWorker() : running(false){};
    virtual ~AbstractWorker() = default;
    void start();
    void stop();

  protected:
    virtual void run() = 0;

  private:
    std::atomic<bool> running;
    std::thread workerThread;
    void threadLoop();
};