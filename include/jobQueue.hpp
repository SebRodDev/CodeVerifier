#pragma once

// Multithreaded safe queue
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "job.hpp"

class JobQueue {
public:
    // creating a sort of producer->consumer queue where the producer must gain access directly to the queue to be push an item

    void push(singleFileJob job) {
        {
            std::lock_guard<std::mutex> lock(mutexLock);
            jobQueue.push(job);
        }

        cv.notify_one();
    }

    std::optional<singleFileJob> pop() {
        std::unique_lock<std::mutex> lock(mutexLock);

        // inlining the function so that we wait until either the queue is no longer empty or it shuts down and this 
        cv.wait(lock, [this] {return !jobQueue.empty() || shutDown; });

        if (jobQueue.empty()) {
            return std::nullopt;
        }

        singleFileJob nextTask = jobQueue.front();
        jobQueue.pop();
        return nextTask;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutexLock);
            shutDown = true;
        }

        cv.notify_all();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutexLock);
        return jobQueue.size();
    }

private:
    std::queue<singleFileJob> jobQueue;
    std::mutex mutexLock;
    std::condition_variable cv;
    bool shutDown = false;
};