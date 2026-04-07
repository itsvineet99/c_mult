#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -m <rows> -k <inner> -n <cols>\n", prog_name);
    fprintf(stderr, "Example: %s -m 1024 -k 1024 -n 1024\n", prog_name);
}

static int parse_positive_int(const char *s, int *out) {
    char *end = NULL;
    long val = 0;

    errno = 0;
    val = strtol(s, &end, 10 /*this is base*/); // string to long conversion 
    if (errno != 0 || end == s || *end != '\0' || val <= 0 || val > 1000000) { // val can't be 0 or neg
        return 0;
    }
    *out = (int)val;
    return 1;
}

// Allocate a rows x cols matrix on the heap as one contiguous block.
static float **alloc_matrix(int rows, int cols) {
    float **m = (float **)malloc((size_t)rows * sizeof(*m));
    float *data = NULL;

    if (!m) {
        return NULL;
    }
    data = (float *)calloc((size_t)rows * (size_t)cols, sizeof(*data));
    if (!data) {
        free(m);
        return NULL;
    }
    for (int i = 0; i < rows; i++) {
        m[i] = data + (size_t)i * (size_t)cols;
    }
    return m;
}

// Free a matrix allocated by alloc_matrix.
static void free_matrix(float **m) {
    if (!m) {
        return;
    }
    free(m[0]);
    free(m);
}

// Compute C = A(m x k) * B(k x n).
static float **matmul_dynamic(int m, int k, int n, float **A, float **B) {
    float **C = alloc_matrix(m, n);
    if (!C) {
        return NULL;
    }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int kk = 0; kk < k; kk++) {
                sum += A[i][kk] * B[kk][j];
            }
            C[i][j] = sum;
        }
    }
    return C;
}

int main(int argc, char *argv[]) {
    int m = 0;
    int k = 0;
    int n = 0;

    // Parse CLI flags: -m (rows of A), -k (cols of A / rows of B), -n (cols of B).
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "-n") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            if (strcmp(argv[i], "-m") == 0) {
                if (!parse_positive_int(argv[i + 1], &m)) {
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "-k") == 0) {
                if (!parse_positive_int(argv[i + 1], &k)) {
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "-n") == 0) {
                if (!parse_positive_int(argv[i + 1], &n)) {
                    print_usage(argv[0]);
                    return 1;
                }
            }
            i++;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (m <= 0 || k <= 0 || n <= 0) {
        print_usage(argv[0]);
        return 1;
    }

    printf("Multiplying: A(%dx%d) * B(%dx%d) = C(%dx%d)\n", m, k, k, n, m, n);

    // Allocate A and B on the heap.
    float **MatrixA = alloc_matrix(m, k);
    float **MatrixB = alloc_matrix(k, n);
    if (!MatrixA || !MatrixB) {
        fprintf(stderr, "Error: failed to allocate matrices A or B.\n");
        free_matrix(MatrixA);
        free_matrix(MatrixB);
        return 1;
    }

    // Initialize A and B with simple, deterministic values.
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) {
            MatrixA[i][j] = (float)((i * k) + j + 1);
        }
    }

    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            MatrixB[i][j] = (float)((i * n) + j + 1);
        }
    }

    // time the operation 
    clock_t start_time = clock();

    // Multiply A and B into a new heap-allocated matrix C.
    float **MatrixC = matmul_dynamic(m, k, n, MatrixA, MatrixB);
    if (!MatrixC) {
        fprintf(stderr, "Error: failed to allocate result matrix C.\n");
        free_matrix(MatrixA);
        free_matrix(MatrixB);
        return 1;
    }

    clock_t end_time = clock();
    double time_spent_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;

    printf("Multiplication complete!\n");
    printf("Time taken: %.3f milliseconds\n", time_spent_ms);

    // print result only to check accuracy.
    // for (int i = 0; i < m; i++) {
    //     for (int j = 0; j < n; j++) {
    //         printf("%.1f\t", MatrixC[i][j]);
    //     }
    //     printf("\n");
    // }

    // Clean up all heap allocations.
    free_matrix(MatrixA);
    free_matrix(MatrixB);
    free_matrix(MatrixC);

    return 0;
}
