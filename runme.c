#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "allocator.h"
int main(){
    uint8_t *b = malloc(800*sizeof(uint8_t));

    //define 5-bit pattern
    uint8_t p1 = 0;
    uint8_t p2 = 1;
    uint8_t p3 = 2;
    uint8_t p4 = 4;
    uint8_t p5 = 8;
    //set pattern in all bytes in heap
    for (int i=0; i<800; i+=5){
        *(b+i) = p1;
        *(b+i+1) = p2;
        *(b+i+2) = p3;
        *(b+i+3) = p4;
        *(b+i+4) = p5;
    }

    // run init
    mm_init(b, 800);

    void* ptr = mm_malloc(70);
    void* ptr2 = mm_malloc(70);
    void* ptr3 = mm_malloc(700);

    printf("\n\nFreeing ptr");
    mm_free(ptr);

    printf("\n\nFreeing ptr2");
    mm_free(ptr2);
    void* ptr4 = mm_malloc(100);
    void* buf[256];
    int arr[100] = {6};
    void* src = arr;
    int y = mm_write(ptr4, 0, src, 4);
    printf("\n WRITTEN DATA: %d", y);

    int x = mm_read(ptr4, 0, buf, 256);
    printf("\n READ DATA: %d", x);

    printf("\nFreeing ptr4");
    mm_free(ptr4);

    
    free(b);
    return 0;
}