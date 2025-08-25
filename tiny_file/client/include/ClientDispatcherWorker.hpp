#pragma once

#include "AbstractWorker.hpp"

class Servicer;

class ClientDispatcherWorker : public AbstractWorker {
  public:
    ClientDispatcherWorker(Servicer& servicer) : servicer(servicer){};
    ~ClientDispatcherWorker() override;
    void run() override;

  private:
    Servicer& servicer;
};