#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FREE 0
#define UNFREE 255

struct Node{
    size_t size; //size of memory block
    uint8_t state; //free or not free
    struct Node *prev; //pointer to previous block
    struct Node *next; //pointer to next block
    int *data; //pointer to data.
};

static struct Node head; 
static struct Node ass; 
//static uint8_t *heapPtr;


// Initialize the allocator over a provided memory block.
// Returns 0 on success, non-zero on failure.
int mm_init(uint8_t *heap, size_t heap_size){
    head.size = 0; head.state = UNFREE; head.prev = NULL; head.next = NULL;
    ass.size = 0; ass.state = UNFREE; ass.prev = NULL; ass.next = NULL;
    if (sizeof(*heap) < (3*sizeof(struct Node))){
        //not enough heap space to do anything
        return -1;
    }
    struct Node *headPtr = &head;
    struct Node *assPtr = &ass;
    struct Node heapNode = {(heap_size - 3*sizeof(struct Node)), FREE, headPtr, assPtr, (int*)heap };
    head.next = &heapNode;
    ass.prev = &heapNode;

    //TODO: write nodes to the heap
    //heapPtr = heap;
    memset(heap,0,heap_size); //set all bytes in the heap to 0
    struct Node *test = (struct Node *) heap;
    *test = head;

    printf("SIZE: %d",head.next->size);
    printf("\n%.20s", *heap);
    return 0;
};

// Allocate a block with ALIGN-byte aligned payload. Returns
// NULL on failure.
void *mm_malloc(size_t size){
    printf("Hello World");
};

// Safely read data from an allocated block at offset bytes into buf.
// Returns the number of bytes read, or -1 if corruption or invalid pointer detected.
int mm_read(void *ptr, size_t offset, void *buf, size_t len){
    printf("Hello World");
};

// Safely write data into an allocated block at offset bytes from src.
// Returns the number of bytes written, or -1 if corruption or invalid pointer detected.
int mm_write(void *ptr, size_t offset, const void *src, size_t len){
    printf("Hello World");
};

// Free a previously-allocated pointer (ignore NULL).
// Must detect double-free.
void mm_free(void *ptr){
    printf("Hello World");

};

int main(){
    int x = sizeof(ass);
    printf("%d",x);
    mm_init(malloc(80), 80);
    return 0;
}