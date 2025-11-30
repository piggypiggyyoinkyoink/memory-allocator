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
    uint8_t state2; //unused
    uint8_t state3; //unused
};

static struct Node head;
static struct Node* headPtr;
static struct Node ass;
static struct Node* assPtr;
static uint8_t *heapPtr;
static uint8_t pattern[5];
static size_t heapSize;
static size_t nodeSize = 120;

size_t get_node_size(struct Node* nodePtr){
    struct Node n = *(nodePtr);

    if (!(n.size == n.size2 && n.size == n.size3 && n.size2 == n.size3)){
        printf("\nWARNING: Inconsistent size values detected in get_node_size\n");
    }
    return n.size;
    size_t correctSize;
    correctSize = (n.size & n.size2) | (n.size & n.size3) | (n.size2 & n.size3);
    n.size = n.size2 = n.size3 = correctSize;
    *(nodePtr) = n;
    return correctSize;
}

struct Node* get_node_prev(struct Node* nodePtr){
    struct Node n = *(nodePtr);
    if (!(n.prev == n.prev2 && n.prev == n.prev3 && n.prev2 == n.prev3)) {
        printf("\nWARNING: Inconsistent prev pointers detected in get_node_prev\n");
    }
    uintptr_t prev1 = (uintptr_t) n.prev;
    uintptr_t prev2 = (uintptr_t) n.prev2;
    uintptr_t prev3 = (uintptr_t) n.prev3;

    uintptr_t correctPrev = (prev1 & prev2) | (prev1 & prev3) | (prev2 & prev3);
    n.prev = n.prev2 = n.prev3 = (struct Node*) correctPrev;
    *(nodePtr) = n;
    return (struct Node*) correctPrev;
}

struct Node* get_node_next(struct Node* nodePtr){
    struct Node n = *(nodePtr);
    if (!(n.next == n.next2 && n.next == n.next3) && !(n.next2 == n.next3)) {
        printf("\nWARNING: Inconsistent next pointers detected in get_node_next\n");

    }
    uintptr_t next1 = (uintptr_t) n.next;
    uintptr_t next2 = (uintptr_t) n.next2;
    uintptr_t next3 = (uintptr_t) n.next3;

    uintptr_t correctNext = (next1 & next2) | (next1 & next3) | (next2 & next3);
    n.next = n.next2 = n.next3 = (struct Node*) correctNext;
    *(nodePtr) = n;
    return (struct Node*) correctNext;
}

void* get_node_data(struct Node* nodePtr){
    struct Node n = *(nodePtr);
    if (!(n.data == n.data2 && n.data == n.data3 && n.data2 == n.data3)) {
        printf("\nWARNING: Inconsistent data pointers detected in get_node_data\n");
    }
    //return n.data;
    uintptr_t data1 = (uintptr_t) n.data;
    uintptr_t data2 = (uintptr_t) n.data2;
    uintptr_t data3 = (uintptr_t) n.data3;
    uintptr_t correctData = (data1 & data2) | (data1 & data3) | (data2 & data3);

    n.data = n.data2 = n.data3 = (void*) correctData;
    *(nodePtr) = n;
    return (void*) correctData;
}

uint8_t get_node_state(struct Node* nodePtr){
    struct Node n = *(nodePtr);
    if (!(n.state == n.state2 && n.state == n.state3 && n.state2 == n.state3)) {
        printf("\nWARNING: Inconsistent state values detected in get_node_state\n");
    }
    
    //return n.state2;
    //uint8_t state1 = n.state;
    uint8_t state = n.state2;
    //uint8_t state3 = n.state3;
    uint8_t count = 0;
    for (int i = 0; i < 8; i++) {
        count += (state >> i) & 1;
    }
    if(count >= 4){
        state = UNFREE;
    }else{
        state = FREE;
    }
    n.state = state;
    *(nodePtr) = n;
    return state;
}



int is_valid_pointer(void* ptr, int sentinels_valid){
    if (sentinels_valid){
        if (((uint8_t*)ptr) < (heapPtr) || ((uint8_t*)ptr) > (heapPtr + heapSize-nodeSize)) {
            printf("\nINVALID POINTER nsi");
            return 0;
        }else{
            return 1;
        }
    }else{
        if ((((uint8_t*)ptr) < (heapPtr+nodeSize)) || (((uint8_t*)ptr) >= (heapPtr + heapSize - 2*nodeSize))) {
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
    head.size = head.size2 = head.size3 =0; head.state = head.state2 = head.state3 = UNFREE; head.prev = head.prev2 = head.prev3 = NULL; head.next = head.next2 = head.next3 = NULL;
    ass.size = ass.size2 = ass.size3 = 0; ass.state = ass.state2 = ass.state3 = UNFREE; ass.prev = ass.prev2 = ass.prev3 = NULL; ass.next = ass.next2 = ass.next3 = NULL;

    if (heap_size < (3*nodeSize)) {
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
    struct Node heapNode = {(heap_size - 3*nodeSize), FREE, NULL, NULL, (int*)heap };
    heapPtr = heap;
    heapNode.size = heapNode.size2 = heapNode.size3 = (heap_size - 3*nodeSize);
    // Initialise pointers to nodes on the heap

    // put the head at the start of the heap
    struct Node *heapHeadPtr = (struct Node *) heap;

    // put the ass  at the end of the heap
    // struct Node *heapAssPtr = (struct Node *) (((uint8_t *)heap + (heap_size-1)) - sizeof(ass) );
    struct Node *heapAssPtr = (struct Node *)((uint8_t *)heap + heap_size - nodeSize);

    // put heapNode in the middle
    struct Node *heapContentsPtr = (struct Node *) ((uint8_t *)heap + nodeSize);

    // set the prev and next pointers for each node
    head.next = head.next2 = head.next3 = heapContentsPtr;
    ass.prev = ass.prev2 = ass.prev3 = heapContentsPtr;
    heapNode.prev = heapNode.prev2 = heapNode.prev3 = heapHeadPtr;
    heapNode.next = heapNode.next2 = heapNode.next3 = heapAssPtr;    
    // set data pointer to point to where the actual data starts
    heapNode.data = heapNode.data2 = heapNode.data3 = (void *)((uint8_t *)heapContentsPtr + nodeSize);
    // write nodes to the heap
    *heapHeadPtr = head;
    *heapAssPtr = ass;
    *heapContentsPtr = heapNode;
    //write head and ass pointers to global vars
    headPtr = heapHeadPtr;
    assPtr = heapAssPtr;
    printf("\nSIZE OF NODE: %zu", nodeSize);
    printf("\nSIZE OF HEAP: %zu", head.next->size);
    int dingus = (uint8_t)((uint8_t*)heapNode.data-(uint8_t*)heapContentsPtr);
    printf("\nDINGUS: %d", dingus);
    return 0;
}




void resize_node(struct Node* nodePtr, size_t size) {
    // initialise current node as n
    struct Node n = *(nodePtr);
    
    if (get_node_size(nodePtr) < size) {
        printf("\nOH FUCK");
    }
    // initialise new node
    struct Node newNode;
    newNode.state = newNode.state2 = newNode.state3 = FREE;
    *(nodePtr) = n;
    newNode.size = newNode.size2 = newNode.size3 = (get_node_size(nodePtr) - (size + nodeSize));
    // update current node size
    n.size = n.size2 = n.size3 = size;
    // get the correct pointer for the new node
    struct Node* newNodePtr = (struct Node*)((uint8_t*)nodePtr + size + nodeSize);
    // initialise data pointer for new node
    newNode.data = newNode.data2 = newNode.data3 = (void *)((uint8_t *)newNodePtr + nodeSize);
    // get next node
    struct Node* nextNodePtr = (struct Node *) get_node_next(nodePtr);
    
    struct Node nextNode = *nextNodePtr;
    // update prev and next pointers
    newNode.prev = newNode.prev2 = newNode.prev3 = nodePtr;
    newNode.next = newNode.next2 = newNode.next3 = nextNodePtr;
    n.next = n.next2 = n.next3 = newNodePtr;
    nextNode.prev = nextNode.prev2 = nextNode.prev3 = newNodePtr;
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
    struct Node* currentNodePtr = (struct Node *) get_node_next(headPtr);
    struct Node currentNode = *currentNodePtr;
    // check all nodes until we find a free one of sufficient size, or reach ass
    while ((!end) && (!found)) {
        printf("\n CURRENT NODE SIZE: %zu", get_node_size(currentNodePtr));
        if (get_node_state(currentNodePtr) == FREE && get_node_size(currentNodePtr) >= size) {
            // resize and allocate this node
            printf("\nMEMORY BLOCK FOUND");
            currentNode.state = currentNode.state2 = currentNode.state3 = UNFREE;  // unfree the node
            *currentNodePtr = currentNode;  // write back to heap
            if (get_node_size(currentNodePtr) > aligned_size + nodeSize) {
                // resize node so there is some heap left for everything else
                // use aligned size to ensure the new node generated by resize_node is also aligned
                printf("\nNODE RESIZED");
                resize_node(currentNodePtr, aligned_size);
            }
            currentNode = *currentNodePtr;
            currentNode.size = currentNode.size2 = currentNode.size3 = size;  // reset the size to the correct size passed into mm_malloc to ensure API conformity

            memset(get_node_data(currentNodePtr), 0, currentNode.size);
            found = 1;
            *currentNodePtr = currentNode;
            printf("\nALLOCATION SUCCESSFUL");
            // get data ptr to return
            void* dataPtr = get_node_data(currentNodePtr);
            return dataPtr;
        } else if (get_node_next(currentNodePtr) == NULL) {
            // no available nodes of sufficient size
            end = 1;
            printf("\nNO FREE MEMORY BLOCKS BIG ENOUGH");
            return NULL;
        } else {
            // try next node
            currentNodePtr = (struct Node *) get_node_next(currentNodePtr);
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
    printf("\nREADING DATA OF LEN %zu AT OFFSET %zu", len, offset);
    ptr = (void*)((uint8_t*)ptr - nodeSize);  // data ptr to node ptr
    // return if pointer invalid
    if (((uint8_t*)ptr) < heapPtr || ((uint8_t*)ptr) > (heapPtr + heapSize)) {
        printf("\nINVALID POINTER IN MM_READ");
        return -1;
    }
    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;
    uint8_t* dataPtr = (uint8_t*)get_node_data(nodePtr);
    uint8_t* buff = (uint8_t*)buf;
    size_t size = get_node_size(nodePtr);
    // nuh uh if block is free or doesnt exist
    if (get_node_state(nodePtr) != UNFREE) {
        printf("CANNOT READ FROM FREE BLOCK");
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
    ptr = (void*)((uint8_t*)ptr - nodeSize);  // data ptr to node ptr
    printf("\nWRITING DATA OF LEN %zu AT OFFSET %zu", len, offset);
    // return if pointer invalid
    if (((uint8_t*)ptr) < heapPtr || ((uint8_t*)ptr) > (heapPtr + heapSize)) {
        printf("\nINVALID POINTER IN MM_WRITE");
        return -1;
    }

    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;
    uint8_t* dataPtr = (uint8_t*)get_node_data(nodePtr);
    uint8_t* source = (uint8_t*)src;
    size_t size = get_node_size(nodePtr);
    // nuh uh if block is free or doesnt exist
    if (get_node_state(nodePtr) != UNFREE) {
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
        // printf("\n%d", *(source+i));
        numBytes++;
    }
    return numBytes;
}




// doesnt actually delete the node but updates the pointers of previous and next nodes so that logically it ceases to exist.
void delete_node(struct Node* nodePtr) {
    struct Node* currentNodePtr = nodePtr;
    struct Node currentNode = *nodePtr;
    // dont delete non-free nodes
    if (get_node_state(currentNodePtr) != FREE) {
        printf("\nCANNOT DELETE NON-FREE NODE");
        return;
    }
    // get prev and next nodes
    struct Node* nextNodePtr = get_node_next(currentNodePtr);
    struct Node nextNode = *nextNodePtr;
    struct Node* prevNodePtr = get_node_prev(currentNodePtr);
    struct Node prevNode = *prevNodePtr;

    // update pointers to bypass deleted node.
    nextNode.prev = nextNode.prev2 = nextNode.prev3 = prevNodePtr;
    prevNode.next = prevNode.next2 = prevNode.next3 = nextNodePtr;
    // write to heap
    *prevNodePtr = prevNode;
    *nextNodePtr = nextNode;
    return;
}




// overwrite a node's data with the 5 byte pattern
void overwrite_data(uint8_t* dataPtr, size_t size) {
    return;
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
    if ((get_node_state(prevNodePtr) == FREE) && (get_node_state(nextNodePtr) == FREE) && (get_node_prev(prevNodePtr) != NULL) && (get_node_next(nextNodePtr) != NULL)) {  // should probs use a separate function to check freeness that can be applied anywhere and takes bitflips into acc.
        // get new size of merged node
        size_t newSize = (size_t)((uint8_t*)get_node_next(nextNodePtr) - (uint8_t*)get_node_data(prevNodePtr));
        // "delete" redundant nodes (just update prev and next ptrs)
        delete_node(currentNodePtr);
        delete_node(nextNodePtr);
        prevNode = *prevNodePtr;
        // update size
        prevNode.size = prevNode.size2 = prevNode.size3 = newSize;
        // update heap
        *prevNodePtr = prevNode;
        // fill with 5B pattern
        overwrite_data(prevNode.data, newSize);//TODO: fix this to use get_node_data

    } else if (get_node_state(prevNodePtr) == FREE) { //something is freeing the head and causing breakages
        size_t newSize = (size_t)((uint8_t*)get_node_next(currentNodePtr) - (uint8_t*)get_node_data(prevNodePtr));
        // "delete" redundant node (just update prev and next ptrs)
        delete_node(currentNodePtr);
        prevNode = *prevNodePtr;  // IMPORTANT: delete_node() changes the pointers of the nodes on the heap so these changes need to be updated in the prevNode variable so they arent overwritten
        // update size
        prevNode.size = prevNode.size2 = prevNode.size3 = newSize;
        // updates heap
        *prevNodePtr = prevNode;
        // fill with 5B pattern
        overwrite_data(prevNode.data, newSize);
    } else if (get_node_state(nextNodePtr) == FREE) {
        size_t newSize = (size_t)((uint8_t*)get_node_next(nextNodePtr) - (uint8_t*)get_node_data(currentNodePtr));

        delete_node(nextNodePtr);
        currentNode = *currentNodePtr;
        currentNode.size = currentNode.size2 = currentNode.size3 = newSize;
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

    ptr = (void*)((uint8_t*)ptr - nodeSize);  // data ptr to node ptr

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
    if (!is_valid_pointer(get_node_prev(nodePtr), 1) || !is_valid_pointer(get_node_next(nodePtr), 1)) {
        printf("\nINVALID PREV/NEXT POINTER");
        return;
    }

    // check for double free
    if (get_node_state(nodePtr) != UNFREE) {
        printf("\nCANNOT FREE NON-UNFREE NODE");
        return;
    }
    // get size and data pointer of node
    size_t size = n.size;
    uint8_t* dataPtr = n.data;
    // free node BEFORE calling merge_node() - IMPORTANT
    n.state = n.state2 = n.state3 = FREE;
    *nodePtr = n;  // update heap

    // Merge with adjacent frees and overwrite data with 5B pattern
    merge_node(nodePtr);

    printf("\nSUCCESSFULLY FREED BLOCK OF SIZE %zu", size);
    return;
}