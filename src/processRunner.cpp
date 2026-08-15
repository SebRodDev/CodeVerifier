#include "ProcessRunner.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <array>
#include <thread>

std::vector<char*> ProcessRunner::stringToC(const std::vector<std::string>& arguments) {
    // since execvp take in a character array
    std::vector<char*> cArguments(arguments.size() + 1);

    for (int i = 0; i < arguments.size(); i++) {
        cArguments[i] = const_cast<char*>(arguments[i].c_str());
    }

    cArguments[arguments.size()] = nullptr;

    return cArguments;
}

ProcessResult ProcessRunner::run(const std::vector<std::string>& arguments, std::chrono::milliseconds timeout) {
    // create a process and then execute it

    int outputPipe[2];

    if (pipe(outputPipe) != 0) {
        return {JobStatus::Crash, "output pipe failed", -1};
    }

    // spawn the process
    pid_t pid = fork();

    if (pid < 0) {
        close(outputPipe[0]); close(outputPipe[1]);
        return {JobStatus::Crash, "process spawning fail", -1};
    }

    // in the child process
    if (pid == 0) {
        // close the end of the pipe that we dont want to ensure when we exit it is closed correctly
        close(outputPipe[0]);
        dup2(outputPipe[1], 1);
        dup2(outputPipe[1], 2);

        close(outputPipe[1]);
        auto cArgumentVersion = stringToC(arguments);
        execvp(cArgumentVersion[0], cArgumentVersion.data());

        // Execvp only fails if the code failed
        exit(-1);
    }

    // mark the read end in the parent as finished
    close(outputPipe[1]);

    // dont want to get stuck blocking and want to make sure if we experience an error that we
    // try again with a new thread which is the reason we have an attempt count
    int flags = fcntl(outputPipe[0], F_GETFL, 0);
    fcntl(outputPipe[0], F_SETFL, flags | O_NONBLOCK);

    std::string output;
    auto timeoutExceeded = std::chrono::steady_clock::now() + timeout;
    bool timeOut = false;
    int status = 0;

    while (true) {
        std::array<char, 4096> buffer{};
        int n;

        // reading whatever is being returned by the process
        while ((n = read(outputPipe[0], buffer.data(), buffer.size())) > 0) {
            output.append(buffer.data(), n);
        }

        // ensuring that the process does not hang
        pid_t processEliminated = waitpid(pid, &status, WNOHANG);

        if (processEliminated == pid) break;

        if (std::chrono::steady_clock::now() >= timeoutExceeded) {
            timeOut = true;
            kill(pid, SIGKILL);

            // ensure the process is cleaned up
            waitpid(pid, &status, 0);
            break;
        }
    }

    // close the read pipe
    close(outputPipe[0]);

    // case where our process timed out when running the operation
    if (timeOut) {
        return {JobStatus::Timeout, "Process timed out", -1};
    }

    if (WIFSIGNALED(status)) {
        return {JobStatus::Crash, "code crashed", -1};
    }

    int exitCode = WEXITSTATUS(status);

    if (exitCode != 0) {
        return {JobStatus::ExecutionError, "Code experienced an error when running: " + output, exitCode};
    }

    return {JobStatus::Success, "Code ran successfully: " + output, exitCode};
}