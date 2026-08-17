#pragma once

#include <string>
#include <vector>
#include "job.hpp"

struct LlmReviewInput {
    Finding finding;
    std::string fileContent;
    std::string localSnippet;
};

class LlmReview {
public:
    // Step 2 only: prepare deterministic prompt payloads, no API call yet.
    static std::string buildPrompt(const LlmReviewInput& input);

    // Step 2 only: parse strict JSON response string into verdict.
    static LlmVerdict parseVerdictJson(const std::string& json);
};