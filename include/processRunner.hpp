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

    static ProcessResult run(const std::vector<std::string>& arguments, std::chrono::milliseconds timeout);
private:
    static std::vector<char*> stringToC(const std::vector<std::string>& arguments);
};