#pragma clang optimize on

#include "std/stddef.h"
#include "std/stdint.h"

void __attribute__((used)) sequential_access(void) {
    int data[4096];
    float coordinates[2048];
    
    #pragma clang loop vectorize(enable)
    #pragma clang loop unroll(enable)
    for (int i = 0; i < 4096; i++) {
        data[i] = i * 2 + 1;
    }
    
    #pragma clang loop distribute(enable)
    for (int i = 0; i < 2048; i++) {
        coordinates[i] = i * 1.5f;
    }
}

void __attribute__((used)) strided_access(void) {
    int matrix[256][256];
    
    #pragma clang loop vectorize_width(4)
    for (int i = 0; i < 256; i += 4) {
        for (int j = 0; j < 256; j += 4) {
            matrix[i][j] = i + j;
        }
    }
}

void __attribute__((used)) random_access(void) {
    int sparse[1024];
    int indices[128];
    
    for (int i = 0; i < 128; i++) {
        int idx = indices[i] % 1024;
        __builtin_prefetch(&sparse[idx], 0, 1);
        sparse[idx] = i;
    }
}

void __attribute__((used)) cache_friendly_structs(void) {
    struct hot_data {
        int frequently_used[16];
    } __attribute__((aligned(64)));
    
    struct hot_data cache_lines[64];
    
    for (int i = 0; i < 64; i++) {
        __builtin_prefetch(&cache_lines[i], 0, 3);
        for (int j = 0; j < 16; j++) {
            cache_lines[i].frequently_used[j] = i * 16 + j;
        }
    }
}