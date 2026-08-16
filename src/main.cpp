#include <iostream>
#include <filesystem>
#include "Scheduler.hpp"

bool isCppFile(const std::filesystem::path& p) {
    // ensure that the files inside are actually cpp related files as this current version of the product only works with cpp files
    static const std::vector<std::string> exts = {".cpp", ".cc", ".cxx", ".h", ".hpp"};
    for (const auto& e : exts) {
        if (p.extension() == e) return true;
    }
    return false;
}

std::vector<std::string> collectAllFiles(const std::string& root) {
    // recursively iterate through all of the files and after ensuring they are a cpp file then they are added to our list of files 
    std::vector<std::string> foundFiles;
    for (const auto& potentialFile : std::filesystem::recursive_directory_iterator(root)) {
        if (potentialFile.is_regular_file() && isCppFile(potentialFile)) {
            foundFiles.push_back(potentialFile.path().string());
        }
    }

    return foundFiles;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return 1;
    }

    Scheduler::Config config;
    std::string directoryRoot = argv[1];
    if (argc >= 3) config.amountThreads = std::stoi(argv[2]);
    if (argc >= 4) config.maxRetries = std::stoi(argv[3]);
    if (argc >= 5) config.maxTimeout = std::chrono::milliseconds(std::stoi(argv[4]));

    std::vector <std::string> allFiles = collectAllFiles(directoryRoot);
    std::cout << "Found " << allFiles.size() << " C++ files under " << directoryRoot << "\n";
    std::cout << "Workers: " << config.amountThreads
        << " | Max retries/job: " << config.maxRetries
        << " | Per-job timeout: " << config.maxTimeout.count() << "ms\n\n";

    if (allFiles.empty()) {
        std::cerr << "No C++ files found in the specified directory." << std::endl;
        return 1;
    }

    Scheduler scheduler(config);
    std::vector<singleFileJobResult> results = scheduler.run(allFiles);

    for (const auto& result : results) {
        std::cout << "File: " << result.job.filePath
            << " | Status: " << static_cast<int>(result.operationStatus)
            << " | Exit Code: " << result.codeExitStatus
            << "\nDebug Output:\n" << result.debugOutput
            << "\n-----------------------------\n";
    }
}