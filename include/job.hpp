#pragma once

#include <string>
#include <vector>

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

enum class LLMDecision {
    PotentialStrongIssue,
    PotentialFalsePositive,
    NeedsRuntimeValidation,
    Inconclusive
};

enum class JobStatus {
    Success,
    Timeout,
    Crash,
    ExecutionError, // happens when the code that was just executed returns an error from a function or something
    LinterError
};

struct Finding {
    std::string filePath;
    int line = 0;
    int column = 0;
    std::string severity;
    std::string checkName;
    std::string message;
    std::string rawLine;
};

struct LlmVerdict {
    LLMDecision decision = LLMDecision::Inconclusive;
    double confidence = 0.0; // 0.0 - 1.0
    std::string reasoning;
    std::string suggestedFix;
};

struct singleFileJobResult {
    singleFileJob job;
    JobStatus operationStatus;

    // basically keeping track of if the program had any prints that the user that is running this might want to know
    std::string debugOutput;
    int codeExitStatus;
    std::vector<Finding> findings;
};