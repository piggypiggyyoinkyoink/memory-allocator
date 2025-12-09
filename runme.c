// Copyright [year] <Copyright Owner>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "./allocator.h"
// ChatGPT-written helper for printing PASS/FAIL
#define TEST(name, expr) do { \
    printf("\nTEST %-40s : ", name); \
    if (expr) printf("PASS\n"); \
    else      printf("FAIL\n"); \
} while (0)



// Utility: nanos
//
static inline double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

//
// Random small size generator
//
static inline size_t rand_size(size_t max) {
    return (rand() % max) + 1;
}

static void* ptrs[1000];
static volatile int STOP_CORRUPTOR = 0;
static size_t BITFLIPS_PER_SEC = 10000000;   // Tune this!
static uint8_t *GLOBAL_HEAP = NULL;
static size_t   GLOBAL_HEAP_SIZE = 0;

//
// ==========  CORRUPTION THREAD  ==========
//
void *heap_corruptor_thread(void *arg) {
    printf("[CORRUPTOR] Flipping ~%zu bits/sec...\n", BITFLIPS_PER_SEC);
    // do BITFLIPS_PER_SEC / 100 bitflips every 10ms
    while (!STOP_CORRUPTOR) {
        for (size_t i = 0; i < BITFLIPS_PER_SEC / 100; i++) {
            size_t pos = rand() % GLOBAL_HEAP_SIZE;
            uint8_t bit = 1u << (rand() % 8);
            GLOBAL_HEAP[pos] ^= bit;     // Flip bit
        }

        // sleep ~10ms
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 10 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }

    printf("[CORRUPTOR] Thread stopped.\n");
    return NULL;
}



//
// Benchmark helpers
//
void bench_malloc(size_t iters) {
    printf("=== Allocation/Free Benchmark (%zu ops) ===\n", iters);
    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        void *p = mm_malloc(64);
        if (!p) {
            printf("Allocation failed on iteration %zu!\n", i);
            return;
        }
        ptrs[i] = p;
    }
    double t1 = now_sec();

    printf("Time: %.6f sec (%.2f ops/sec)\n",
        t1 - t0, (iters) / (t1 - t0));
}

void bench_free(size_t iters) {
    printf("=== Free Benchmark (%zu ops) ===\n", iters);
    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        mm_free(ptrs[i]);
    }
    double t1 = now_sec();

    printf("Time: %.6f sec (%.2f ops/sec)\n",
        t1 - t0, (iters) / (t1 - t0));
}

void bench_realloc(size_t iters) {
    printf("=== Realloc Benchmark (%zu ops) ===\n", iters);

    void *p = mm_malloc(16);
    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        p = mm_realloc(p, (i % 256) + 1);
        if (!p) {
            printf("Realloc failed on iteration %zu!\n", i);
            return;
        }
    }
    double t1 = now_sec();
    mm_free(p);

    printf("Time: %.6f sec (%.2f ops/sec)\n",
        t1 - t0, (iters) / (t1 - t0));
}

void bench_rw(size_t iters, size_t size) {
    printf("=== Read/Write Benchmark (%zu ops, block %zu bytes) ===\n",
        iters, size);

    void *p = mm_malloc(size);
    uint8_t *buf = malloc(size);
    memset(buf, 0xAB, size);

    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        mm_write(p, 0, buf, size);
        mm_read(p, 0, buf, size);
    }
    double t1 = now_sec();

    mm_free(p);
    free(buf);

    printf("Time: %.6f sec (%.2f ops/sec)\n",
        t1 - t0, (iters) / (t1 - t0));
}

void bench_mixed(size_t iters) {
    printf("=== Mixed Workload Benchmark (%zu ops) ===\n", iters);

    void *ptrs[1000] = {0};

    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        int op = rand() % 4;
        if (op == 0) {  // alloc
            size_t idx = rand() % 1000;
            if (ptrs[idx] == NULL)
                ptrs[idx] = mm_malloc(rand_size(512));

        } else if (op == 1) {  // write
            size_t idx = rand() % 1000;
            if (ptrs[idx])
                mm_write(ptrs[idx], 0, "ABCDEF", 6);
        } else if (op == 2) {  // realloc
            continue;
            size_t idx = rand() % 1000;
            if (ptrs[idx])
                ptrs[idx] = mm_realloc(ptrs[idx], rand_size(512));
        } else {  // free
            size_t idx = rand() % 1000;
            if (ptrs[idx]) {
                mm_free(ptrs[idx]);
                ptrs[idx] = NULL;
            }
        }
    }
    double t1 = now_sec();

    // cleanup
    for (int i = 0; i < 1000; i++)
        if (ptrs[i])
            mm_free(ptrs[i]);

    printf("Time: %.6f sec (%.2f ops/sec)\n",
        t1 - t0, (iters) / (t1 - t0));
}

//
// MAIN
//
int main() {
    uint8_t *heap = malloc(16000*sizeof(uint8_t));

    // define 5-bit pattern
    uint8_t p1 = 0;
    uint8_t p2 = 1;
    uint8_t p3 = 2;
    uint8_t p4 = 4;
    uint8_t p5 = 8;
    // set pattern in all bytes in heap
    for (int i = 0; i < 16000; i += 5) {
        *(heap+i) = p1;
        *(heap+i+1) = p2;
        *(heap+i+2) = p3;
        *(heap+i+3) = p4;
        *(heap+i+4) = p5;
    }

    // run init
    mm_init(heap, 16000);

    printf("\n\nAllocating ptr");
    void* ptr = mm_malloc(70);

    printf("\n\nAllocating ptr2");
    void* ptr2 = mm_malloc(70);

    printf("\n\nAllocating ptr3");
    void* ptr3 = mm_malloc(700);
    if (ptr3 != NULL) {
        mm_free(ptr3);
    }
    printf("\n\nFreeing ptr");
    mm_free(ptr);

    printf("\n\nFreeing ptr2");
    mm_free(ptr2);

    printf("\n\nAllocating ptr4");
    void* ptr4 = mm_malloc(4);

    printf("\n\nAllocating ptr5");
    void* ptr5 = mm_malloc(120);

    printf("\n\nAllocating ptr6");
    void* ptr6 = mm_malloc(101);
    printf("\n\nFreeing ptr5");
    mm_free(ptr5);

    printf("\n\nAllocating ptr8");
    void* ptr8 = mm_malloc(56000);  // should fail
    if (ptr8 != NULL) {
        mm_free(ptr8);
    }
    printf("\n\nFreeing ptr6");
    mm_free(ptr6);
    void* buf[256];
    int arr[100] = {6};
    void* src = arr;
    int y = mm_write(ptr4, 0, src, 4);
    printf("\n WRITTEN DATA: %d", y);

    int x = mm_read(ptr4, 0, buf, 4);
    printf("\n READ DATA: %d", x);

    printf("\n\nFreeing ptr4");
    mm_free(ptr4);
    printf("\n\nFreeing ptr4 (double-free test)");
    mm_free(ptr4);
    mm_free((void*)2);

    // ChatGPT-written Tests
    printf("\n[2] Testing basic allocation...\n");


    void *a = mm_malloc(32);
    TEST("mm_malloc(32) != NULL", a != NULL);

    /* ----------------------------------------------------
        TEST 2: Write and Read
    ---------------------------------------------------- */
    printf("\n[3] Testing write/read...\n");

    char msg[] = "Hello world!";
    int numB1 = mm_write(a, 5, msg, sizeof(msg));
    a = mm_realloc(a, 64);
    char bufff[32];
    memset(bufff, 0, sizeof(bufff));
    int numB2 = mm_read(a, 5, bufff, sizeof(msg));
    printf("\nSIZEOF: %zu", sizeof(msg));
    printf("\nNUMB1: %d", numB1);
    printf("\nNUMB2: %d", numB2);
    TEST("mm_write/mm_read store correct data", strcmp(bufff, msg) == 0);

    /* ----------------------------------------------------
        TEST 3: Allocate multiple blocks
    ---------------------------------------------------- */
    printf("\n[4] Multi-block allocation...\n");

    void *b = mm_malloc(64);
    void *c = mm_malloc(128);
    TEST("Second alloc != NULL", b != NULL);
    TEST("Third alloc != NULL", c != NULL);

    mm_write(b, 0, "TESTING-123", 12);

    char temp[16];
    memset(temp, 0, sizeof(temp));
    mm_read(b, 0, temp, 12);
    puts(temp);
    TEST("Second block stores correct data", strcmp(temp, "TESTING-123") == 0);

    /* ----------------------------------------------------
        TEST 4: Free a block
    ---------------------------------------------------- */
    printf("\n[5] Freeing blocks...\n");

    mm_free(b);
    TEST("Free b (no crash)", 1);

    /* ----------------------------------------------------
        TEST 5: Double-free detection
    ---------------------------------------------------- */
    printf("\n[6] Double-free detection...\n");

    // Should print error internally, but not crash
    mm_free(b);
    TEST("Double-free does not crash", 1);

    /* ----------------------------------------------------
        TEST 6: Adjacent block merging
    ---------------------------------------------------- */
    printf("\n[7] Testing merge of free blocks...\n");

    // Free a and c, leaving three adjacent free blocks
    mm_free(a);
    mm_free(c);

    // Now re-allocate a large block to see if merge worked
    void *big = mm_malloc(512);
    TEST("Allocator merged free blocks (big alloc != NULL)", big != NULL);
    mm_free(big);
    /* ----------------------------------------------------
        TEST 7: Out-of-bounds write protection
    ---------------------------------------------------- */
    printf("\n[8] Testing out-of-bounds write protection...\n");

    void *xx = mm_malloc(16);

    uint8_t bigbuff[64];
    memset(bigbuff, 0xAA, sizeof(bigbuff));

    // Behavior here depends on your implementation:
    // It should ignore, fail, or clamp the write,
    // but MUST NOT crash.
    mm_write(xx, 0, bigbuff, 9999);

    char rbuff[64];
    memset(rbuff, 0, sizeof(rbuff));
    mm_read(xx, 0, rbuff, 16);
    mm_free(xx);
    // Not checking content — only checking no crash
    TEST("Out-of-bounds write does not crash", 1);

    /* ----------------------------------------------------
        TEST 8: NULL pointer behavior
    ---------------------------------------------------- */
    printf("\n[9] Testing NULL behavior...\n");

    mm_free(NULL);                          // should do nothing
    mm_write(NULL, 0, "A", 1);              // should do nothing
    mm_read(NULL, 0, rbuff, 16);            // should do nothing

    TEST("NULL operations do not crash", 1);
    // realloc tests

    void* p11 = mm_malloc(100);
    void* p21 = mm_malloc(20);
    void* p31 = mm_malloc(30);
    void* p41 = mm_malloc(40);

    p31 = mm_realloc(p31, 50);
    TEST("Realloc in the middle", p31 != NULL);
    p11 = mm_realloc(p11, 31);
    TEST("Realloc decreases size", p11 != NULL);
    void* p51 = mm_malloc(200);

    if (p51 != NULL) {
        mm_free(p51);
    }
    if (p41 != NULL) {
        mm_free(p41);
    }
    if (p11 != NULL) {
        mm_free(p11);
    }

    p21 = mm_realloc(p21, 80);
    TEST("Realloc increases size", p21 != NULL);
    if (p31 != NULL) {
        mm_free(p31);
    }
    if (p21 != NULL) {
        mm_free(p21);
    }
    /* ----------------------------------------------------
        TEST 9: Allocate entire heap
    ---------------------------------------------------- */
    printf("\n[10] Testing full-heap allocation...\n");

    void *big2 = mm_malloc(15880);
    TEST("Large alloc after freeing everything", big2 != NULL);

    printf("\n======== ALL TESTS COMPLETE ========\n");
    free(heap);

    const size_t HEAP_SIZE = 64 * 1024 * 1024;  // 64 MB
    heap = malloc(HEAP_SIZE);

    if (!heap) {
        printf("Failed to allocate test heap.\n");
        return 1;
    }

    printf("Initializing heap (%zu bytes)...\n", HEAP_SIZE);
    if (mm_init(heap, HEAP_SIZE) != 0) {
        printf("mm_init failed!\n");
        return 1;
    }

    srand(12345);

    bench_malloc(1000);
    bench_free(1000);
    bench_realloc(1e3);
    bench_rw(5e3, 256);
    bench_mixed(5e3);

    free(heap);
    GLOBAL_HEAP = malloc(HEAP_SIZE);
    GLOBAL_HEAP_SIZE = HEAP_SIZE;

    if (!GLOBAL_HEAP) {
        printf("Failed to allocate heap.\n");
        return 1;
    }

    printf("Initializing allocator...\n");
    if (mm_init(GLOBAL_HEAP, HEAP_SIZE) != 0) {
        printf("mm_init failed!\n");
        return 1;
    }

    srand(12345);

    // Start bit-flipping thread
    pthread_t corruptor;
    pthread_create(&corruptor, NULL, heap_corruptor_thread, NULL);

    //
    // --- RUN BENCHMARKS ---
    //
    bench_malloc(1000);
    bench_free(1000);
    bench_realloc(1e3);
    bench_rw(5e3, 256);
    bench_mixed(5e3);

    // Stop corruptor thread
    STOP_CORRUPTOR = 1;
    pthread_join(corruptor, NULL);

    free(GLOBAL_HEAP);
    return 0;
}
