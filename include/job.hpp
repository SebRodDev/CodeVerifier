#pragma once

#include <string>

// represents the operations that are happening on one file at a time in our processing system
struct singleFileJob {
    std::string filePath;
    int currentAttempt;

    // default constructor
    singleFileJob() {
        filePath = "";
        currentAttempt = 0;
    }

    // want to make sure the user passes in an actual string for the filepath otherwise we know its not a valid file path
    explicit singleFileJob(std::string filePath) : filePath(filePath) {
        currentAttempt = 0;
    }
};

enum class JobStatus {
    Success,
    Timeout,
    Crash,
    ExecutionError // happens when the code that was just executed returns an error from a function or something
};

struct singleFileJobResult {
    singleFileJob job;
    JobStatus operationStatus;

    // basically keeping track of if the program had any prints that the user that is running this might want to know
    std::string debugOutput;
    int codeExitStatus;
};