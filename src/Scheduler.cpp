#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <utility>

#include "Scheduler.hpp"
#include "processRunner.hpp"

static std::vector<Finding> parseClangTidyFindings(const std::string& output) {
    std::vector<Finding> findings;
    std::istringstream stream(output);
    std::string line;

    // Typical format:
    // /path/file.cpp:12:8: warning: message text [check-name]
    static const std::regex findingPattern(
        R"(^(.+?):([0-9]+):([0-9]+):\s+(warning|error|note):\s+(.*?)(?:\s+\[([^\]]+)\])?$)"
    );

    while (std::getline(stream, line)) {
        std::smatch match;
        if (!std::regex_match(line, match, findingPattern)) {
            continue;
        }

        Finding finding;
        finding.filePath = match[1].str();
        finding.line = std::stoi(match[2].str());
        finding.column = std::stoi(match[3].str());
        finding.severity = match[4].str();
        finding.message = match[5].str();
        finding.checkName = match[6].matched ? match[6].str() : "";
        finding.rawLine = line;
        findings.push_back(std::move(finding));
    }

    return findings;
}

static std::string statusMessage(JobStatus status) {
    switch (status) {
    case JobStatus::Success:
        return "Success";
    case JobStatus::Crash:
        return "Crash";
    case JobStatus::Timeout:
        return "Timeout";
    case JobStatus::ExecutionError:
        return "Execution Error";
    case JobStatus::LinterError:
        return "Linter Error";
    }

    return "UNKNOWN STATUS";
}

std::vector<singleFileJobResult> Scheduler::run(const std::vector<std::string>& filePaths) {
    if (filePaths.empty()) {
        return {};
    }

    totalJobs.store(filePaths.size());
    completedJobs.store(0);

    for (const auto& path : filePaths) {
        queue.push(singleFileJob(path));
    }

    std::vector<std::thread> threads(config.amountThreads);;

    for (int i = 0; i < config.amountThreads; i++) {
        threads[i] = std::thread(&Scheduler::threadLoop, this);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // waiting until all threads are done and then returning the results
    std::lock_guard<std::mutex> lock(resultsMutex);
    return results;
}

void Scheduler::threadLoop() {
    ProcessRunner processRunner;

    while (true) {
        std::optional<singleFileJob> jobOpt = queue.pop();
        if (!jobOpt.has_value()) {
            return;
        }

        singleFileJob job = jobOpt.value();
        // mark that we are on a new attempt
        job.currentAttempt++;

        std::vector<std::string> arguments = {
            "clang-tidy",
            "-checks=clang-analyzer-*",
            job.filePath,
            "--",
            "-std=c++17"
        };

        // required in order to run clang-tidy
        if (const char* sdkRoot = std::getenv("SDKROOT"); sdkRoot != nullptr && *sdkRoot != '\0') {
            arguments.emplace_back("-isysroot");
            arguments.emplace_back(sdkRoot);
        }

        ProcessRunner::ProcessResult result = ProcessRunner::run(arguments, config.maxTimeout);
        std::vector<Finding> findings = parseClangTidyFindings(result.output);

        if (result.jobStatus == JobStatus::ExecutionError && !findings.empty()) {
            result.jobStatus = JobStatus::LinterError;
        }

        bool shouldRetry = (result.jobStatus == JobStatus::Timeout || result.jobStatus == JobStatus::Crash);

        // if we experienced a crash or timed out we should retry to see if it was something actually
        // wrong with the code or not
        if (shouldRetry && job.currentAttempt < config.maxRetries) {
            std::cerr << "[retry] " << job.filePath
                << " attempt " << job.currentAttempt
                << " failed (" << static_cast<int>(result.jobStatus)
                << "), requeuing\n";

            queue.push(job);
            continue;
        }

        // we've reached a point where we are done so lock it and add the result to the vector
        {
            //std::cout << "[result] " << job.filePath
            //    << " attempt " << job.currentAttempt
            //    << " finished with status: " << statusMessage(result.jobStatus) << " Error Code: " << result.exitCode
            //    << "\n";

            if (result.jobStatus == JobStatus::Success) {
                result.output = "Code ran successfully with no output";
            }

            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back({job, result.jobStatus, result.output, result.exitCode, findings});
        }

        const size_t finished = completedJobs.fetch_add(1) + 1;
        if (finished >= totalJobs.load()) {
            queue.shutdown();
        }
    }
}
