#include "RegistrationWorker.hpp"

/**
 * @brief Runs the registration worker.
 *
 * Continuously registers clients and dispatches their requests.
 */
void RegistrationWorker::run() {
    while (true) {
        registrationVisitor.reg();
        registrationVisitor.dispatch();
    }
}