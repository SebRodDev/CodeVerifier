// LlmApi.hpp
#pragma once
#include <string>

struct LlmApiResult {
    bool success = false;
    long httpStatus = 0;
    std::string body;         // raw HTTP response body on success
    std::string errorMessage; // curl error or non-200 explanation on failure
};

class LlmApi {
public:
    // Sends `promptText` to Gemini and returns the raw HTTP response body.
    static LlmApiResult callGemini(const std::string& promptText);
    static std::string extractGeminiText(const std::string& envelopeJson, bool& success);

private:
    static size_t writeCallback(char* contents, size_t size, size_t nmemb, void* userp);
};