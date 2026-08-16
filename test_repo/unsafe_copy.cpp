#include <cstring>

// Intentionally vulnerable: no bounds check before copying into a
// fixed-size stack buffer.
void copy_into_buffer(const char* user_input) {
    char buffer[64];
    strcpy(buffer, user_input); // CWE-120: classic stack buffer overflow
}

int main() {
    copy_into_buffer("hello");
    return 0;
}
