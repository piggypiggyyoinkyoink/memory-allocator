#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "allocator.h"
#include <assert.h>
#include <string.h>
// Helper for printing PASS/FAIL
#define TEST(name, expr) do { \
    printf("\nTEST %-40s : ", name); \
    if (expr) printf("PASS\n"); \
    else      printf("FAIL\n"); \
} while(0)
int main(){
    uint8_t *heap = malloc(800*sizeof(uint8_t));

    //define 5-bit pattern
    uint8_t p1 = 0;
    uint8_t p2 = 1;
    uint8_t p3 = 2;
    uint8_t p4 = 4;
    uint8_t p5 = 8;
    //set pattern in all bytes in heap
    for (int i=0; i<800; i+=5){
        *(heap+i) = p1;
        *(heap+i+1) = p2;
        *(heap+i+2) = p3;
        *(heap+i+3) = p4;
        *(heap+i+4) = p5;
    }

    // run init
    mm_init(heap, 800);

    printf("\n\nAllocating ptr");
    void* ptr = mm_malloc(70);

    printf("\n\nAllocating ptr2");
    void* ptr2 = mm_malloc(70);

    printf("\n\nAllocating ptr3");
    void* ptr3 = mm_malloc(700);
    if (ptr3 != NULL){
        mm_free(ptr3);
    }
    printf("\n\nFreeing ptr");
    mm_free(ptr);

    printf("\n\nFreeing ptr2");
    mm_free(ptr2);

    printf("\n\nAllocating ptr4");
    void* ptr4 = mm_malloc(100);

    printf("\n\nAllocating ptr5");
    void* ptr5 = mm_malloc(120);

    printf("\n\nAllocating ptr6");
    void* ptr6 = mm_malloc(101);
    printf("\n\nFreeing ptr5");
    mm_free(ptr5);

    printf("\n\nAllocating ptr8");
    void* ptr8 = mm_malloc(560);//should fail
    if (ptr8 != NULL){
        mm_free(ptr8);
    }
    printf("\n\nFreeing ptr6");
    mm_free(ptr6);
    void* buf[256];
    int arr[100] = {6};
    void* src = arr;
    int y = mm_write(ptr4, 0, src, 4);
    printf("\n WRITTEN DATA: %d", y);

    int x = mm_read(ptr4, 0, buf, 256);
    printf("\n READ DATA: %d", x);

    printf("\n\nFreeing ptr4");
    mm_free(ptr4);
    printf("\n\nFreeing ptr4 (double-free test)");
    mm_free(ptr4);
    mm_free((void*)2);
    printf("\nHeapPtr: %p, PTR: %p, ptr2: %p, ptr4: %p, ptr5: %p", heap, ptr, ptr2, ptr4, ptr5);
    free(heap);

    printf("\n[2] Testing basic allocation...\n");

    void *a = mm_malloc(32);
    TEST("mm_malloc(32) != NULL", a != NULL);

    /* ----------------------------------------------------
       TEST 2: Write and Read
    ---------------------------------------------------- */
    printf("\n[3] Testing write/read...\n");

    char msg[] = "Hello world!";
    int numB1 = mm_write(a, 5, msg, sizeof(msg));

    char bufff[32];
    memset(bufff, 0, sizeof(bufff));
    int numB2 = mm_read(a, 5,bufff, sizeof(msg));
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

    mm_write(b,0, "TESTING-123", 12);

    char temp[16];
    memset(temp, 0, sizeof(temp));
    mm_read(b,0, temp, 12);
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
    mm_write(xx,0, bigbuff, 9999);

    char rbuff[64];
    memset(rbuff, 0, sizeof(rbuff));
    mm_read(xx,0, rbuff, 16);
    mm_free(xx);
    // Not checking content — only checking no crash
    TEST("Out-of-bounds write does not crash", 1);

    /* ----------------------------------------------------
       TEST 8: NULL pointer behavior
    ---------------------------------------------------- */
    printf("\n[9] Testing NULL behavior...\n");

    mm_free(NULL);                    // should do nothing
    mm_write(NULL,0, "A", 1);           // should do nothing
    mm_read(NULL,0, rbuff, 16);         // should do nothing

    TEST("NULL operations do not crash", 1);

    /* ----------------------------------------------------
       TEST 9: Allocate entire heap
    ---------------------------------------------------- */
    printf("\n[10] Testing full-heap allocation...\n");

    void *big2 = mm_malloc(800 - 3 * 40);
    TEST("Large alloc after freeing everything", big2 != NULL);

    printf("\n======== ALL TESTS COMPLETE ========\n");
    return 0;
}