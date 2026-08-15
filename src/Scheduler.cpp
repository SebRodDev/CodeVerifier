#include <filesystem>
#include <string>
#include <vector>

#include "Scheduler.hpp"
#include "processRunner.hpp"

static bool isCppFile(const std::filesystem::path& p) {
    // ensure that the files inside are actually cpp related files as this current version of the product only works with cpp files
    static const std::vector<std::string> exts = {".cpp", ".cc", ".cxx", ".h", ".hpp"};
    for (const auto& e : exts) {
        if (p.extension() == e) return true;
    }
    return false;
}

static std::vector<std::string> collectAllFiles(const std::string& root) {
    // recursively iterate through all of the files and after ensuring they are a cpp file then they are added to our list of files 
    std::vector<std::string> foundFiles;
    for (const auto& potentialFile : std::filesystem::recursive_directory_iterator(root)) {
        if (potentialFile.is_regular_file() && isCppFile(potentialFile)) {
            foundFiles.push_back(potentialFile.path().string());
        }
    }

    return files;
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
    }

    return "UNKNOWN STATUS";
}
