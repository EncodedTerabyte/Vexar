
#pragma clang optimize on
#pragma clang fp contract(fast)

static void __attribute__((
    always_inline,
    hot,
    const,
    returns_nonnull,
    nothrow
)) optimization_template_1(void) {}

static void __attribute__((
    cold,
    noreturn,
    noinline
)) error_path_template(void) {
    __builtin_trap();
}

struct __attribute__((packed, aligned(64))) cache_friendly {
    int frequently_accessed[16];  // One cache line
};

void __attribute__((ms_abi)) windows_call_hint(void) {}
void __attribute__((sysv_abi)) standard_call_hint(void) {}