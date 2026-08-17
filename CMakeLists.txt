cmake_minimum_required(VERSION 3.16)
project(codeDebugger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

add_executable(codeDebugger
    src/processRunner.cpp
    src/Scheduler.cpp
    src/LlmReview.cpp
    src/LlmApi.cpp
    src/main.cpp
)

target_include_directories(codeDebugger PRIVATE include)
target_link_libraries(codeDebugger PRIVATE Threads::Threads CURL::libcurl)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)
target_link_libraries(codeDebugger PRIVATE nlohmann_json::nlohmann_json)