#include <cstdlib>

int* make_and_free() {
    int* p = new int(42);
    delete p;
    return p; // CWE-416: returning a pointer to freed memory
}

int main() {
    int* val = make_and_free();
    return *val; // use-after-free
}
