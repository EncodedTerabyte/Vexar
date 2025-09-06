#pragma clang optimize on
#pragma clang fp contract(fast)
#pragma clang attribute push (__attribute__((always_inline)), apply_to = function)

#define NULL ((void*)0)
#define uintptr_t unsigned long long

void __attribute__((const, used, hot)) value_range_training(void) {
    int screen_x = 1920;
    int screen_y = 1080;
    __builtin_assume(screen_x >= 0 && screen_x <= 7680);
    __builtin_assume(screen_y >= 0 && screen_y <= 4320);
    
    char buffer[4096];
    __builtin_assume(__builtin_strlen(buffer) < 4096);
    
    void* handle = (void*)0x10000;
    __builtin_assume(handle != NULL);
    __builtin_assume((uintptr_t)handle > 0x1000);
    
    float coord = 123.45f;
    __builtin_assume(coord >= -10000.0f && coord <= 10000.0f);
}

#pragma clang attribute pop