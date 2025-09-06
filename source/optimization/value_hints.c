#pragma clang optimize on
#pragma clang fp contract(fast)

#define NULL ((void*)0)
#define uintptr_t unsigned long long

void __attribute__((const, used, hot)) screen_coordinates(void) {
    int x = 1920, y = 1080;
    __builtin_assume(x >= 0 && x <= 7680);
    __builtin_assume(y >= 0 && y <= 4320);
}

void __attribute__((const, used)) handle_ranges(void) {
    void* hwnd = (void*)0x10000;
    void* hdc = (void*)0x20000;
    __builtin_assume(hwnd != NULL);
    __builtin_assume(hdc != NULL);
    __builtin_assume((uintptr_t)hwnd > 0x1000);
    __builtin_assume((uintptr_t)hdc > 0x1000);
}

void __attribute__((const, used)) string_lengths(void) {
    char buffer[4096];
    char title[256];
    char message[512];
    __builtin_assume(__builtin_strlen(buffer) < 4096);
    __builtin_assume(__builtin_strlen(title) < 256);
    __builtin_assume(__builtin_strlen(message) < 512);
}

void __attribute__((const, used)) numeric_ranges(void) {
    int count = 100;
    float coord = 123.45f;
    double precision = 3.14159;
    __builtin_assume(count >= 0 && count <= 10000);
    __builtin_assume(coord >= -10000.0f && coord <= 10000.0f);
    __builtin_assume(precision >= -1000000.0 && precision <= 1000000.0);
}