#include <string>

std::string greet(const std::string& name) {
    return "Hello, " + name + "!";
}

int main() {
    std::string msg = greet("Sebas");
    return static_cast<int>(msg.size());
}
