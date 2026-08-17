#include "LlmApi.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

size_t LlmApi::writeCallback(char* contents, size_t size, size_t nmemb, void* userp) {
    // libcurl calls this repeatedly as response bytes arrive; we just
    // append each chunk onto the std::string passed in via userp.
    size_t totalSize = size * nmemb;
    static_cast<std::string*>(userp)->append(contents, totalSize);
    return totalSize;
}

std::string LlmApi::extractGeminiText(const std::string& envelopeJson, bool& success) {
    success = false;

    try {
        nlohmann::json parsed = nlohmann::json::parse(envelopeJson);

        if (!parsed.contains("candidates") || parsed["candidates"].empty()) {
            return "No candidates returned (possibly blocked by safety filters)";
        }

        const auto& firstCandidate = parsed["candidates"][0];
        const std::string text = firstCandidate
            .at("content")
            .at("parts")[0]
            .at("text")
            .get<std::string>();

        success = true;
        return text;

    }
    catch (const nlohmann::json::exception& e) {
        return std::string("Failed to parse Gemini envelope: ") + e.what();
    }
}

LlmApiResult LlmApi::callGemini(const std::string& promptText) {
    LlmApiResult result;

    const char* apiKey = std::getenv("GEMINI_API_KEY");
    if (apiKey == nullptr) {
        result.errorMessage = "GEMINI_API_KEY environment variable not set";
        return result;
    }

    // Wrap the plain prompt text into Gemini's required request shape.
    nlohmann::json requestBody = {
        {"contents", {{
            {"parts", {{
                {"text", promptText}
            }}}
        }}}
    };

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        result.errorMessage = "curl_easy_init failed";
        return result;
    }

    const std::string url =
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash:generateContent";
    const std::string bodyStr = requestBody.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("x-goog-api-key: ") + apiKey).c_str());

    std::string responseBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        result.errorMessage = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return result;
    }

    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    result.httpStatus = httpStatus;
    result.body = responseBuffer;
    result.success = (httpStatus == 200);

    if (!result.success) {
        result.errorMessage = "Gemini API returned HTTP " + std::to_string(httpStatus);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}