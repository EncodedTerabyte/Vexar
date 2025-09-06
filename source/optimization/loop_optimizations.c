#pragma clang optimize on

void __attribute__((used)) vectorizable_loops(void) {
    float input[2048];
    float output[2048];
    
    #pragma clang loop vectorize(enable)
    #pragma clang loop vectorize_width(8)
    for (int i = 0; i < 2048; i++) {
        output[i] = input[i] * 2.0f + 1.0f;
    }
    
    #pragma clang loop unroll_count(8)
    for (int i = 0; i < 1024; i++) {
        input[i] = i * 0.5f;
    }
}

void __attribute__((used)) reduction_patterns(void) {
    int data[4096];
    int sum = 0;
    int max_val = 0;
    
    #pragma clang loop vectorize(enable)
    for (int i = 0; i < 4096; i++) {
        sum += data[i];
    }
    
    #pragma clang loop vectorize(enable)
    for (int i = 0; i < 4096; i++) {
        if (data[i] > max_val) {
            max_val = data[i];
        }
    }
}

void __attribute__((used)) nested_loops(void) {
    int matrix_a[256][256];
    int matrix_b[256][256];
    int result[256][256];
    
    #pragma clang loop distribute(enable)
    for (int i = 0; i < 256; i++) {
        #pragma clang loop vectorize(enable)
        for (int j = 0; j < 256; j++) {
            result[i][j] = matrix_a[i][j] + matrix_b[i][j];
        }
    }
}