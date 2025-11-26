// Copyright [year] <Copyright Owner>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iso646.h>
#include "./allocator.h"
#define FREE 0
#define UNFREE 255

struct Node{
    size_t size;  // size of memory block
    uint8_t state;  // free or not free
    struct Node *prev;  // pointer to previous block
    struct Node *next;  // pointer to next block
    void *data;  // pointer to data.
    size_t size2;  // size of memory block
    struct Node *prev2;  // pointer to previous block
    struct Node *next2;  // pointer to next block
    void *data2;  // pointer to data.
    size_t size3;  // size of memory block
    struct Node *prev3;  // pointer to previous block
    struct Node *next3;  // pointer to next block
    void *data3;  // pointer to data.
    uint8_t* a; //unused
    uint8_t* b; //unused
};

static struct Node head;
static struct Node ass;
static uint8_t *heapPtr;
static uint8_t pattern[5];
static size_t heapSize;

int is_valid_pointer(void* ptr, int sentinels_valid){
    if (sentinels_valid){
        if (((uint8_t*)ptr) < (heapPtr) || ((uint8_t*)ptr) > (heapPtr + heapSize-sizeof(struct Node))) {
            printf("\nINVALID POINTER nsi");
            return 0;
        }else{
            return 1;
        }
    }else{
        if (((uint8_t*)ptr) < (heapPtr+sizeof(struct Node)-1) || ((uint8_t*)ptr) >= (heapPtr + heapSize - 2*sizeof(struct Node))) {
            printf("\nINVALID POINTER si");
            return 0;
        }else{
            return 1;
        }
    }
}

// Initialize the allocator over a provided memory block.
// Returns 0 on success, non-zero on failure.
int mm_init(uint8_t *heap, size_t heap_size) {
    // initialise head and ass senitnels
    head.size = head.size2 = head.size3 =0; head.state = UNFREE; head.prev = head.prev2 = head.prev3 = NULL; head.next = head.next2 = head.next3 = NULL;
    ass.size = ass.size2 = ass.size3 = 0; ass.state = UNFREE; ass.prev = ass.prev2 = ass.prev3 = NULL; ass.next = ass.next2 = ass.next3 = NULL;

    if (heap_size < (3*sizeof(struct Node))) {
        // not enough heap space to do anything
        printf("Uh oh");
        return -1;
    }
    // Get 5-byte pattern
    // needs updating to some kinda majority vote thingy to handle shenanigans
    for (int i = 0; i < 5; i++) {
        uint8_t byte = *(heap+i);
        pattern[i] = byte;
    }
    // DEBUG: print pattern
    for (int i = 0; i < 5; i++) {
        printf("\nPATTERN:%x", pattern[i]);
    }
    // set heapSize global var
    heapSize = heap_size;

    // initialise initial data node
    struct Node heapNode = {(heap_size - 3*sizeof(struct Node)), FREE, NULL, NULL, (int*)heap };
    heapPtr = heap;
    heapNode.size = heapNode.size2 = heapNode.size3 = (heap_size - 3*sizeof(struct Node));
    // Initialise pointers to nodes on the heap

    // put the head at the start of the heap
    struct Node *heapHeadPtr = (struct Node *) heap;

    // put the ass  at the end of the heap
    // struct Node *heapAssPtr = (struct Node *) (((uint8_t *)heap + (heap_size-1)) - sizeof(ass) );
    struct Node *heapAssPtr = (struct Node *)((uint8_t *)heap + heap_size - sizeof(struct Node));

    // put heapNode in the middle
    struct Node *heapContentsPtr = (struct Node *) ((uint8_t *)heap + sizeof(head));

    // set the prev and next pointers for each node
    head.next = head.next2 = head.next3 = heapContentsPtr;
    ass.prev = ass.prev2 = ass.prev3 = heapContentsPtr;
    heapNode.prev = heapNode.prev2 = heapNode.prev3 = heapHeadPtr;
    heapNode.next = heapNode.next2 = heapNode.next3 = heapAssPtr;
    // set data pointer to point to where the actual data starts
    heapNode.data = heapNode.data2 = heapNode.data3 = (void *)((uint8_t *)heapContentsPtr + sizeof(heapNode));
    // write nodes to the heap
    *heapHeadPtr = head;
    *heapAssPtr = ass;
    *heapContentsPtr = heapNode;

    printf("\nSIZE OF NODE: %zu", sizeof(struct Node));
    printf("\nSIZE OF HEAP: %zu", head.next->size);
    return 0;
}




void resize_node(struct Node* nodePtr, size_t size) {
    // initialise current node as n
    struct Node n = *(nodePtr);
    if (n.size < size) {
        printf("\nOH FUCK");
    }
    // initialise new node
    struct Node newNode;
    newNode.state = FREE;
    newNode.size = (n.size - (size + sizeof(struct Node)));
    // update current node size
    n.size = size;
    // get the correct pointer for the new node
    struct Node* newNodePtr = (struct Node*)((uint8_t*)nodePtr + size + sizeof(struct Node));
    // initialise data pointer for new node
    newNode.data = (void *)((uint8_t *)newNodePtr + sizeof(struct Node));
    // get next node
    struct Node* nextNodePtr = (struct Node *) n.next;
    struct Node nextNode = *nextNodePtr;
    // update prev and next pointers
    newNode.prev = nodePtr;
    newNode.next = nextNodePtr;
    n.next = newNodePtr;
    nextNode.prev = newNodePtr;
    // write to heap
    *nodePtr = n;
    *newNodePtr = newNode;
    *nextNodePtr = nextNode;
    return;
}




// Allocate a block with ALIGN-byte aligned payload. Returns
// NULL on failure.
void *mm_malloc(size_t size) {
    printf("\nALLOCATING BLOCK OF SIZE %zu", size);
    if (size < 1) {
        return NULL;
    }
    size_t aligned_size;
    if (size %40 != 0) {
        aligned_size = size + (40 - (size%40));
    } else {
        aligned_size = size;
    }
    uint8_t end = 0;
    uint8_t found = 0;
    // start at the first data node
    struct Node* currentNodePtr = (struct Node *) head.next;
    struct Node currentNode = *currentNodePtr;
    // check all nodes until we find a free one of sufficient size, or reach ass
    while ((!end) && (!found)) {
        printf("\n CURRENT NODE SIZE: %zu", currentNode.size);
        if (currentNode.state == FREE && currentNode.size >= size) {
            // resize and allocate this node
            printf("\nMEMORY BLOCK FOUND");
            currentNode.state = UNFREE;  // unfree the node
            *currentNodePtr = currentNode;  // write back to heap
            if (currentNode.size > aligned_size + sizeof(struct Node)) {
                // resize node so there is some heap left for everything else
                // use aligned size to ensure the new node generated by resize_node is also aligned
                resize_node(currentNodePtr, aligned_size);
            }
            currentNode = *currentNodePtr;
            currentNode.size = size;  // reset the size to the correct size passed into mm_malloc to ensure API conformity
            memset(currentNode.data, 0, currentNode.size);
            found = 1;
            *currentNodePtr = currentNode;
            printf("\nALLOCATION SUCCESSFUL");
            // get data ptr to return
            void* dataPtr = currentNode.data;
            return dataPtr;
        } else if (currentNode.next == NULL) {
            // no available nodes of sufficient size
            end = 1;
            printf("\nNO FREE MEMORY BLOCKS BIG ENOUGH");
            return NULL;
        } else {
            // try next node
            currentNodePtr = (struct Node *) currentNode.next;
            currentNode = *currentNodePtr;
            printf("\nNEXT BLOCK");
        }
    }
    printf("RETURNING NULL");
    return NULL;
}




// Safely read data from an allocated block starting at offset bytes into buf.
// Returns the number of bytes read, or -1 if corruption or invalid pointer detected.
int mm_read(void *ptr, size_t offset, void *buf, size_t len) {
    ptr = (void*)((uint8_t*)ptr - sizeof(struct Node));  // data ptr to node ptr
    // return if pointer invalid
    if (((uint8_t*)ptr) < heapPtr || ((uint8_t*)ptr) > (heapPtr + heapSize)) {
        printf("\nINVALID POINTER IN MM_READ");
        return -1;
    }
    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;
    uint8_t* dataPtr = (uint8_t*)n.data;
    uint8_t* buff = (uint8_t*)buf;
    size_t size = n.size;
    // nuh uh if block is free or doesnt exist
    if (n.state != UNFREE) {
        printf("CANNOT WRITE TO FREE BLOCK");
        return -1;
    }
    // nuh uh if buf isnt big enough to store the data
    /*
    if (len < (size-offset)){
        printf("BUF TOO SMALL");

        return -1;
    }
    */

    // read data 1 byte at a time
    int numBytes = 0;
    // apply offset
    dataPtr += offset;

    // for (size_t i = offset; i<(size-offset); i++){//should this be len instead of size-offset??
    for (size_t i = 0; i < len; i++) {  // should this be len instead of size-offset??
        *(buff+i) = *(dataPtr+i);
        // printf("\n%d", *(buff+i));
        numBytes++;
    }
    return numBytes;
}




// Safely write data into an allocated block starting at offset bytes from src.
// Returns the number of bytes written, or -1 if corruption or invalid pointer detected.
int mm_write(void *ptr, size_t offset, const void *src, size_t len) {
    ptr = (void*)((uint8_t*)ptr - sizeof(struct Node));  // data ptr to node ptr

    // return if pointer invalid
    if (((uint8_t*)ptr) < heapPtr || ((uint8_t*)ptr) > (heapPtr + heapSize)) {
        printf("\nINVALID POINTER IN MM_WRITE");
        return -1;
    }

    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;
    uint8_t* dataPtr = (uint8_t*)n.data;
    uint8_t* source = (uint8_t*)src;
    size_t size = n.size;
    // nuh uh if block is free or doesnt exist
    if (n.state != UNFREE) {
        printf("\nCANNOT WRITE TO FREE BLOCK");
        return -1;
    }
    // nuh uh if len is bigger than the size of the block
    if ((len > (size-offset)) || (offset > size)) {
        printf("\nBLOCK TOO SMALL");
        return -1;
    }
    // write data 1 byte at a time
    int numBytes = 0;
    // for (size_t i = offset; i<(size-offset); i++){//should this be len instead of size-offset??

    // apply offset
    dataPtr += offset;
    for (size_t i = 0; i < (len); i++) {
        *(dataPtr+i) = *(source+i);
        numBytes++;
    }
    return numBytes;
}




// doesnt actually delete the node but updates the pointers of previous and next nodes so that logically it ceases to exist.
void delete_node(struct Node* nodePtr) {
    struct Node* currentNodePtr = nodePtr;
    struct Node currentNode = *nodePtr;
    // dont delete non-free nodes
    if (currentNode.state != FREE) {
        printf("\nCANNOT DELETE NON-FREE NODE");
        return;
    }
    // get prev and next nodes
    struct Node* nextNodePtr = currentNode.next;
    struct Node nextNode = *nextNodePtr;
    struct Node* prevNodePtr = currentNode.prev;
    struct Node prevNode = *prevNodePtr;

    // update pointers to bypass deleted node.
    nextNode.prev = prevNodePtr;
    prevNode.next = nextNodePtr;
    // write to heap
    *prevNodePtr = prevNode;
    *nextNodePtr = nextNode;
    return;
}




// overwrite a node's data with the 5 byte pattern
void overwrite_data(uint8_t* dataPtr, size_t size) {
    // to ensure pattern is properly aligned
    if (size == 0) {
        return;
    }
    int x = ((intptr_t)dataPtr - (intptr_t)heapPtr) % 5;
    for (size_t i = 0; i < size; ++i) {
        dataPtr[i] = pattern[(i + x) % 5];
    }
        return;
    }




void merge_node(struct Node* nodePtr) {
    struct Node* currentNodePtr = nodePtr;
    struct Node currentNode = *currentNodePtr;
    struct Node* nextNodePtr = currentNode.next;
    struct Node nextNode = *nextNodePtr;
    struct Node* prevNodePtr = currentNode.prev;
    struct Node prevNode = *prevNodePtr;
    if ((prevNode.state == FREE) && (nextNode.state == FREE) && (prevNode.prev != NULL) && (nextNode.next != NULL)) {  // should probs use a separate function to check freeness that can be applied anywhere and takes bitflips into acc.
        // get new size of merged node
        size_t newSize = (size_t)((uint8_t*)nextNode.next - (uint8_t*)prevNode.data);
        // "delete" redundant nodes (just update prev and next ptrs)
        delete_node(currentNodePtr);
        delete_node(nextNodePtr);
        prevNode = *prevNodePtr;
        // update size
        prevNode.size = newSize;
        // update heap
        *prevNodePtr = prevNode;
        // fill with 5B pattern
        overwrite_data(prevNode.data, newSize);

    } else if (prevNode.state == FREE) {
        // get new size of merged node
        size_t newSize = (size_t)((uint8_t*)currentNode.next - (uint8_t*)prevNode.data);

        // "delete" redundant node (just update prev and next ptrs)
        delete_node(currentNodePtr);
        prevNode = *prevNodePtr;  // IMPORTANT: delete_node() changes the pointers of the nodes on the heap so these changes need to be updated in the prevNode variable so they arent overwritten
        // update size
        prevNode.size = newSize;
        // updates heap
        *prevNodePtr = prevNode;
        // fill with 5B pattern
        overwrite_data(prevNode.data, newSize);
    } else if (nextNode.state == FREE) {
        size_t newSize = (size_t)((uint8_t*)nextNode.next - (uint8_t*)currentNode.data);

        delete_node(nextNodePtr);
        currentNode = *currentNodePtr;
        currentNode.size = newSize;
        *currentNodePtr = currentNode;
        overwrite_data(currentNode.data, newSize);
    } else {
        // if no merging to be done, just overwrite with 5B pattern
        overwrite_data(currentNode.data, currentNode.size);
    }
    return;
}




// Free a previously-allocated pointer (ignore NULL).
// Must detect double-free.
void mm_free(void *ptr) {
    ptr = (void*)((uint8_t*)ptr - sizeof(struct Node));  // data ptr to node ptr

    // check for null pointer
    if (ptr == NULL) {
        // fuck off if null
        printf("\nCANNOT FREE NULL POINTER");
        return;
    }
    // return if pointer is invalid
    if (!is_valid_pointer(ptr, 0)) {
        printf("\nINVALID POINTER");
        return;
    }

    // Get the node
    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;

    // return if prev or next pointers are invalid
    printf("\n%p", n.prev);
    printf("\n%p", n.next);
    if (!is_valid_pointer(n.prev, 1) || !is_valid_pointer(n.next, 1)) {
        printf("\nINVALID PREV/NEXT POINTER");
        return;
    }

    // check for double free
    if (n.state != UNFREE) {
        printf("\nCANNOT FREE NON-UNFREE NODE");
        return;
    }
    // get size and data pointer of node
    size_t size = n.size;
    uint8_t* dataPtr = n.data;

    // free node BEFORE calling merge_node() - IMPORTANT
    n.state = FREE;
    *nodePtr = n;  // update heap

    // Merge with adjacent frees and overwrite data with 5B pattern
    merge_node(nodePtr);

    printf("\nSUCCESSFULLY FREED BLOCK OF SIZE %zu", size);
    return;
}