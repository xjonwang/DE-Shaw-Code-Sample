#pragma once

#include "AbstractWorker.hpp"

class Servicer;

class ClientFileCompressorWorker : public AbstractWorker {
  public:
    ClientFileCompressorWorker(int workerId, Servicer& servicer)
        : workerId(workerId), servicer(servicer){};
    ~ClientFileCompressorWorker() override;
    void run() override;

  private:
    int workerId;
    Servicer& servicer;
};