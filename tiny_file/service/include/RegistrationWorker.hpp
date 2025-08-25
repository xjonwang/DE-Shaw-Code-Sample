#pragma once

#include "RegistrationVisitor.hpp"

/**
 * @class RegistrationWorker
 * @brief Handles the registration of clients.
 * 
 * Spins in infinite loop scanning for registration messages and client request messages.
 * Handles the registration messages and enqueues client request messages for scheduler to dispatch later.
 */
class RegistrationWorker {
  public:
    RegistrationWorker(){};
    void run();

  private:
    RegistrationVisitor& registrationVisitor = RegistrationVisitor::getInstance();
};