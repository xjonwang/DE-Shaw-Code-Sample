#include "ArgParse.hpp"
#include "CreditSchedulingStrategy.hpp"
#include "ISchedulingStrategy.hpp"
#include "RegistrationWorker.hpp"
#include "RoundRobinSchedulingStrategy.hpp"
#include "Scheduler.hpp"
#include "ServiceFileCompressorWorker.hpp"
#include "ShmManager.hpp"
#include "Util.hpp"

#include <iostream>
#include <thread>
#include <vector>

static constexpr size_t NUM_WORKERS = 4;

enum class SchedulerType { ROUND_ROBIN, CREDIT };

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("TinyFile");

    program.add_argument("--n_sms")
        .help("Number of shared memory segments")
        .required()
        .scan<'i', int>();

    program.add_argument("--sms_size")
        .help("Shared memory segment size in bytes")
        .required()
        .scan<'i', int>();

    program.add_argument("--scheduler")
        .help("Set the scheduler algorithm to CREDIT or ROUND_ROBIN")
        .default_value(SchedulerType::ROUND_ROBIN)
        .action([](const std::string& value) {
            if (value == "ROUND_ROBIN") {
                return SchedulerType::ROUND_ROBIN;
            } else if (value == "CREDIT") {
                return SchedulerType::CREDIT;
            } else {
                THROW_RUNTIME_ERROR("Invalid scheduler: " + value);
            }
        });

    int numShmSegments, shmSegmentSize;
    SchedulerType schedulerType;
    try {
        program.parse_args(argc, argv);

        numShmSegments = program.get<int>("--n_sms");
        shmSegmentSize = program.get<int>("--sms_size");
        schedulerType = program.get<SchedulerType>("--scheduler");

        std::cout << "Number of shared memory segments: " << numShmSegments << "\n";
        std::cout << "Shared memory segment size: " << shmSegmentSize << " bytes\n";
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Initialize shared memory manager
    ShmManager::init(numShmSegments, shmSegmentSize, 32, true);

    // Initialize scheduler
    switch (schedulerType) {
    case SchedulerType::ROUND_ROBIN:
        Scheduler::init(NUM_WORKERS, std::make_unique<RoundRobinSchedulingStrategy>());
        break;
    case SchedulerType::CREDIT:
        Scheduler::init(NUM_WORKERS, std::make_unique<CreditSchedulingStrategy>());
        break;
    default:
        THROW_RUNTIME_ERROR("Invalid scheduler");
        break;
    }

    // Start registration worker and file compression workers
    RegistrationWorker registrationWorker;
    std::thread registrationThread(&RegistrationWorker::run, &registrationWorker);
    std::vector<ServiceFileCompressorWorker> fileCompressorWorkers;
    std::vector<std::thread> fileCompressorThreads;
    fileCompressorWorkers.reserve(NUM_WORKERS);
    fileCompressorThreads.reserve(NUM_WORKERS);
    for (size_t i = 0; i < NUM_WORKERS; i++) {
        fileCompressorWorkers.emplace_back(i);
        fileCompressorThreads.emplace_back(&ServiceFileCompressorWorker::run,
                                           &fileCompressorWorkers[i]);
    }
    registrationThread.join();
    for (size_t i = 0; i < NUM_WORKERS; i++) {
        fileCompressorThreads[i].join();
    }
    return 0;
}