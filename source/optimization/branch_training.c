#pragma clang optimize on

#include "std/stddef.h"
#include "std/stdint.h"

void __attribute__((hot, used)) success_patterns(void) {
    int window_created = 1;
    int message_sent = 1;
    int handle_valid = 1;
    
    if (__builtin_expect(window_created, 1)) {
        __builtin_prefetch((void*)0x1000, 0, 3);
    }
    
    if (__builtin_expect(message_sent, 1)) {
        __builtin_prefetch((void*)0x2000, 0, 2);
    }
    
    if (__builtin_expect(handle_valid, 1)) {
        return;
    }
}

void __attribute__((cold, used)) error_patterns(void) {
    int allocation_failed = 0;
    int invalid_parameter = 0;
    int system_error = 0;
    
    if (__builtin_expect(allocation_failed, 0)) {
        __builtin_trap();
    }
    
    if (__builtin_expect(invalid_parameter, 0)) {
        __builtin_unreachable();
    }
    
    if (__builtin_expect(system_error, 0)) {
        __builtin_trap();
    }
}

void __attribute__((used)) initialization_patterns(void) {
    int first_run = 0;
    int already_initialized = 1;
    int config_loaded = 1;
    
    if (__builtin_expect(first_run, 0)) {
        __builtin_prefetch((void*)0x10000, 1, 1);
    }
    
    if (__builtin_expect(already_initialized, 1)) {
        __builtin_prefetch((void*)0x20000, 0, 3);
    }
    
    if (__builtin_expect(config_loaded, 1)) {
        return;
    }
}