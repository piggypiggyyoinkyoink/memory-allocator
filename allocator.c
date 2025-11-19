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
    void *data; //pointer to data.
};

static struct Node head; 
static struct Node ass; 
static uint8_t *heapPtr;
static uint8_t pattern[5];


// Initialize the allocator over a provided memory block.
// Returns 0 on success, non-zero on failure.
int mm_init(uint8_t *heap, size_t heap_size){
    //initialise head and ass
    head.size = 0; head.state = UNFREE; head.prev = NULL; head.next = NULL;
    ass.size = 0; ass.state = UNFREE; ass.prev = NULL; ass.next = NULL;

    if (heap_size < (3*sizeof(struct Node))){
        //not enough heap space to do anything
        printf("Uh oh");
        return -1;
    }
    //Get 5-byte pattern (needs updating to some kinda majority vote thingy to handle shenanigans)
    for(int i = 0; i<5; i++){
        uint8_t byte = *(heap+i);
        pattern[i] = byte;
    }
    //DEBUG: print pattern
    for(int i = 0; i<5; i++){
        printf("\nPATTERN:%x", pattern[i]);

    }

    //initialise initial data node
    struct Node heapNode = {(heap_size - 3*sizeof(struct Node)), FREE, NULL, NULL, (int*)heap };

    heapPtr = heap;


    //Initialise pointers to nodes on the heap

    //put the head at the start of the heap
    struct Node *heapHeadPtr = (struct Node *) heap;
    
    //put the ass  at the end of the heap
    struct Node *heapAssPtr = (struct Node *) (((uint8_t *)heap + (heap_size-1)) - sizeof(ass) );
    
    //put heapNode in the middle
    struct Node *heapContentsPtr = (struct Node *) ((uint8_t *)heap + sizeof(head));
    
    //set the prev and next pointers for each node
    head.next = heapContentsPtr;
    ass.prev = heapContentsPtr;
    heapNode.prev = heapHeadPtr;
    heapNode.next = heapAssPtr;
    //set data pointer to point to where the actual data starts
    heapNode.data = (void *)((uint8_t *)heapContentsPtr + sizeof(heapNode));
    //write nodes to the heap
    *heapHeadPtr = head;
    *heapAssPtr = ass;
    *heapContentsPtr = heapNode;

    
    
    /* Example code:
    
    
    //update data for node
    *(int *)heapContentsPtr->data = 42;
    (*(char *)heapContentsPtr->data) ='a';

    //write to heap
    struct Node *test = (struct Node *) heap;
    *test = head;


    //read from heap
    struct Node *nnn = (struct Node *) heap;
    printf("DINGUS: %d\n", nnn->size);
    */

    printf("SIZE: %d",head.next->size);
    return 0;
};

// Allocate a block with ALIGN-byte aligned payload. Returns
// NULL on failure.
void *mm_malloc(size_t size){
    printf("Hello World");
};

// Safely read data from an allocated block starting at offset bytes into buf.
// Returns the number of bytes read, or -1 if corruption or invalid pointer detected.
int mm_read(void *ptr, size_t offset, void *buf, size_t len){
    printf("Hello World");
};

// Safely write data into an allocated block starting at offset bytes from src.
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
    //reading from the heap outside the init function
    /*
    //get head node
    struct Node *nnn = (struct Node *) heapPtr;
    struct Node node = *nnn;

    //print data stored in next node
    printf("DINGUS: %d\n", *(int *)((node.next)->data));
    printf("DINGUS: %c\n", *(char *)((node.next)->data));

    */
    free(b);
    return 0;
}