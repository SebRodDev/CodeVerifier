#pragma once

#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

#include "jobQueue.hpp"
#include "job.hpp"

class Scheduler {
public:
    struct Config {
        int amountThreads = 4;
        int maxRetries = 3;
        std::chrono::milliseconds maxTimeout{5000};
    };

    explicit Scheduler(Config config) : config(config) {}

    std::vector<singleFileJobResult> run(const std::vector<std::string>& filePaths);

    void threadLoop();

private:
    Config config;
    JobQueue queue;

    std::mutex resultsMutex;
    std::vector<singleFileJobResult> results;
    std::atomic<size_t> totalJobs{0};
    std::atomic<size_t> completedJobs{0};
};