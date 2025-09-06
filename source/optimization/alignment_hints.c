#pragma clang optimize on

#include "std/stddef.h"
#include "std/stdint.h"

void __attribute__((used)) pointer_alignment(void) {
    void* aligned_4 = __builtin_assume_aligned((void*)0x1000, 4);
    void* aligned_8 = __builtin_assume_aligned((void*)0x2000, 8);
    void* aligned_16 = __builtin_assume_aligned((void*)0x4000, 16);
    void* aligned_32 = __builtin_assume_aligned((void*)0x8000, 32);
    void* aligned_64 = __builtin_assume_aligned((void*)0x10000, 64);
}

void __attribute__((used)) typed_alignment(void) {
    int* int_array = __builtin_assume_aligned((int*)0x1000, 16);
    float* float_array = __builtin_assume_aligned((float*)0x2000, 16);
    double* double_array = __builtin_assume_aligned((double*)0x4000, 32);
    
    for (int i = 0; i < 64; i += 4) {
        int_array[i] = i;
        float_array[i] = i * 1.5f;
        double_array[i] = i * 2.5;
    }
}

void __attribute__((used)) struct_alignment(void) {
    struct aligned_struct {
        int data[16];
    } __attribute__((aligned(64)));
    
    struct aligned_struct items[128];
    
    for (int i = 0; i < 128; i++) {
        __builtin_prefetch(&items[i], 0, 3);
        for (int j = 0; j < 16; j++) {
            items[i].data[j] = i * 16 + j;
        }
    }
}