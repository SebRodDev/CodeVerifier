#pragma once

#include <string>
#include <vector>
#include <chrono>

#include "job.hpp"

class ProcessRunner {
public:
    struct ProcessResult {
        JobStatus jobStatus;
        std::string output;
        int exitCode;
    };

    ProcessResult run(const std::vector<std::string>& arguments, std::chrono::milliseconds timeout);
private:
    std::vector<char*> stringToC(const std::vector<std::string>& arguments);
};