#pragma clang optimize on

#include "std/stddef.h"
#include "std/stdint.h"

void __attribute__((always_inline, used)) hot_path_functions(void) {
    int frequent_operation_1(void);
    int frequent_operation_2(void);
    void cleanup_function(void);
}

void __attribute__((cold, used)) error_handling_functions(void) {
    void handle_allocation_error(void);
    void handle_invalid_parameter(void);
    void handle_system_failure(void);
}

void __attribute__((const, used)) pure_functions(void) {
    int math_operation(int a, int b);
    float coordinate_transform(float x, float y);
    int hash_function(const char* str);
}

void __attribute__((noinline, used)) large_functions(void) {
    void complex_initialization(void);
    void bulk_data_processing(void);
    void comprehensive_validation(void);
}

void __attribute__((used)) calling_conventions(void) {
    void __attribute__((ms_abi)) windows_api_call(void);
    void __attribute__((sysv_abi)) standard_call(void);
    void __attribute__((fastcall)) fast_call(void);
}