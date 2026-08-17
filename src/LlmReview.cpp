#include "LlmReview.hpp"

#include <nlohmann/json.hpp>
#include <sstream>

namespace {

    LLMDecision parseDecision(const std::string& decisionText) {
        if (decisionText == "PotentialStrongIssue") return LLMDecision::PotentialStrongIssue;
        if (decisionText == "PotentialFalsePositive") return LLMDecision::PotentialFalsePositive;
        if (decisionText == "NeedsRuntimeValidation") return LLMDecision::NeedsRuntimeValidation;
        return LLMDecision::Inconclusive;
    }

} // namespace

std::string LlmReview::buildPrompt(const LlmReviewInput& input) {
    // Build the "Finding" section as real JSON via the library instead of
    // hand-concatenating strings -- this guarantees correct escaping even
    // if a message/rawLine contains quotes, backslashes, etc.
    nlohmann::json findingJson = {
        {"filePath", input.finding.filePath},
        {"line", input.finding.line},
        {"column", input.finding.column},
        {"severity", input.finding.severity},
        {"checkName", input.finding.checkName},
        {"message", input.finding.message},
        {"rawLine", input.finding.rawLine}
    };

    std::ostringstream prompt;
    prompt << "You are adjudicating a static-analysis finding.\n";
    prompt << "Return ONLY strict JSON with this schema:\n";
    prompt << R"({"decision":"PotentialStrongIssue|PotentialFalsePositive|NeedsRuntimeValidation|Inconclusive","confidence":0.0,"reasoning":"...","suggestedFix":"..."})";
    prompt << "\nDo not include markdown, comments, or extra keys.\n\n";

    prompt << "Finding:\n" << findingJson.dump() << "\n\n";

    prompt << "Local snippet:\n-----BEGIN_SNIPPET-----\n"
        << input.localSnippet
        << "\n-----END_SNIPPET-----\n\n";

    prompt << "File content:\n-----BEGIN_FILE-----\n"
        << input.fileContent
        << "\n-----END_FILE-----\n";

    return prompt.str();
}

LlmVerdict LlmReview::parseVerdictJson(const std::string& json) {
    LlmVerdict verdict;
    verdict.decision = LLMDecision::Inconclusive;

    try {
        nlohmann::json parsed = nlohmann::json::parse(json);

        std::string decisionText = parsed.at("decision").get<std::string>();
        double confidence = parsed.at("confidence").get<double>();
        std::string reasoning = parsed.at("reasoning").get<std::string>();
        // suggestedFix treated as optional -- default to empty if the
        // model omits it rather than failing the whole verdict.
        std::string suggestedFix = parsed.value("suggestedFix", "");

        verdict.decision = parseDecision(decisionText);
        verdict.confidence = std::clamp(confidence, 0.0, 1.0);
        verdict.reasoning = reasoning;
        verdict.suggestedFix = suggestedFix;

        if (verdict.decision == LLMDecision::Inconclusive && decisionText != "Inconclusive") {
            verdict.reasoning = "Unknown decision value: " + decisionText;
            verdict.confidence = 0.0;
            verdict.suggestedFix.clear();
        }
    }
    catch (const nlohmann::json::exception& e) {
        verdict.reasoning = std::string("Failed to parse LLM response: ") + e.what();
        verdict.confidence = 0.0;
    }

    return verdict;
}