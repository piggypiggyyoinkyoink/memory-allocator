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
    size_t size2;  // size of memory block
    struct Node *prev2;  // pointer to previous block
    size_t size3;  // size of memory block
    struct Node *prev3;  // pointer to previous block
    uint16_t checksum;  // checksum
    uint8_t* a;
    uint16_t b;
    uint8_t c;

};

// do we need the next and data ptrs? we can just infer them from size and sizeof(node)

static struct Node head;
static struct Node* headPtr;
static struct Node ass;
static struct Node* assPtr;
static uint8_t *heapPtr;
static uint8_t pattern[5];
static size_t heapSize;

struct Node* fix_ptr(struct Node* nodePtr, int direction);


int is_valid_pointer(void* ptr, int sentinels_valid) {
    if (sentinels_valid) {
        if (((uint8_t*)ptr) < (heapPtr)
        || ((uint8_t*)ptr) > (heapPtr + heapSize-sizeof(struct Node))
        ) {
            printf("\nINVALID POINTER nsi");
            return 0;
        } else {
            return 1;
        }
    } else {
        if (
            ((uint8_t*)ptr) < (heapPtr+sizeof(struct Node)-1)
            || ((uint8_t*)ptr) >= (heapPtr + heapSize - 2*sizeof(struct Node))
        ) {
            printf("\nINVALID POINTER si");
            return 0;
        } else {
            return 1;
        }
    }
}



// ensure metadata is written to the heap correctly
int check_node(struct Node* nodePtr, struct Node node) {
    struct Node n = *nodePtr;
    if (
        (n.size != node.size)
        || (n.size2 != node.size2)
        || (n.size3 != node.size3)
        || (n.prev != node.prev)
        || (n.prev2 != node.prev2)
        || (n.prev3 != node.prev3)
        // || (n.next != node.next)
        // || (n.next2 != node.next2)
        // || (n.next3 != node.next3)
        // || (n.data != node.data)
        // || (n.data2 != node.data2)
        // || (n.data3 != node.data3)
        || (n.state != node.state)
        || (n.checksum != node.checksum)
    ) {
        return 0;
    } else {
        return 1;
    }
}



size_t get_node_size(struct Node* nodePtr) {
    struct Node n = *(nodePtr);
    size_t correctSize;
    correctSize = (n.size & n.size2) | (n.size & n.size3) | (n.size2 & n.size3);
    n.size = n.size2 = n.size3 = correctSize;
    *(nodePtr) = n;
    return correctSize;
}

struct Node* get_node_prev(struct Node* nodePtr) {
    struct Node n = *(nodePtr);
    uintptr_t prev1 = (uintptr_t) n.prev;
    uintptr_t prev2 = (uintptr_t) n.prev2;
    uintptr_t prev3 = (uintptr_t) n.prev3;

    uintptr_t correctPrev = (prev1 & prev2) | (prev1 & prev3) | (prev2 & prev3);
    if (is_valid_pointer((void*)correctPrev, 1) || (void*)correctPrev == NULL) {

        n.prev = n.prev2 = n.prev3 = (struct Node*) correctPrev;
        do {
            *(nodePtr) = n;
        } while (!check_node(nodePtr, n));

    } else {
        return fix_ptr(nodePtr, 1);
    }
    return (struct Node*) correctPrev;
}

struct Node* get_node_next(struct Node* nodePtr) {
    struct Node n = *(nodePtr);
    // uintptr_t next1 = (uintptr_t) n.next;
    // uintptr_t next2 = (uintptr_t) n.next2;
    // uintptr_t next3 = (uintptr_t) n.next3;

    // uintptr_t correctNext = (next1 & next2) | (next1 & next3) | (next2 & next3);
    // // printf("\ncorrectNext: %p", (struct Node*) correctNext);
    // if (is_valid_pointer((void*)correctNext, 1) || (void*)correctNext == NULL) {
    //     n.next = n.next2 = n.next3 = (struct Node*) correctNext;
    //     do {
    //         *(nodePtr) = n;
    //     } while (!check_node(nodePtr, n));
    // } else {
    //     return fix_ptr(nodePtr, 0);
    // }
    if ((uintptr_t)nodePtr >= ((uintptr_t)assPtr - sizeof(struct Node))){ 
        return NULL;
    }
    if (get_node_size(nodePtr)%40 == 0){
        size_t tmp = get_node_size(nodePtr);
        struct Node* correctNext = (struct Node*)((uint8_t*)nodePtr + sizeof(struct Node) + tmp);
        return correctNext;
    }
    size_t tmp = get_node_size(nodePtr) + (40 - get_node_size(nodePtr)%40);
    struct Node* correctNext = (struct Node*)((uint8_t*)nodePtr + sizeof(struct Node) + tmp);
    if (!is_valid_pointer((void*)correctNext, 1)){
        return assPtr;
    }
    return correctNext;
}

struct Node* fix_ptr(struct Node* nodePtr, int direction){
    // direction = 0: 
    // Corrupted next ptr: try working backwards from ass
    // to get to nodePtr and repair
    printf("\nFIXING PTR");
    if (direction == 0){
        struct Node* currentNodePtr = assPtr;
        struct Node* prevNodePtr = NULL;
        while ((get_node_prev(currentNodePtr) != nodePtr) && (currentNodePtr != headPtr) && (is_valid_pointer(currentNodePtr, 1))){
            prevNodePtr = currentNodePtr;
            currentNodePtr = get_node_prev(currentNodePtr);
        }
        struct Node n = *(nodePtr);

        // working backwards is also corrputed:
        // try to repair or replace corrupted area
        if (!is_valid_pointer(currentNodePtr, 1)) {
            
            // Look where the corrupted node should be
            struct Node* corrputedNodePtr = (struct Node*)(nodePtr + sizeof(struct Node) + get_node_size(nodePtr));
            // check there is room to rebuild corrupted node

            if ((size_t)((prevNodePtr) - (corrputedNodePtr)) < sizeof(struct Node)){
                // not enough room to rebuild corrupted node
                struct Node newNextNode = *(prevNodePtr);
                // bypass corrupted area
                // n.next = n.next2 = n.next3 = prevNodePtr;
                newNextNode.prev = newNextNode.prev2 = newNextNode.prev3 = nodePtr;
                do {
                    *(nodePtr) = n;
                } while (!check_node(nodePtr, n));
                do {
                    *(prevNodePtr) = newNextNode;
                } while (!check_node(prevNodePtr, newNextNode));
                // prevNodePtr is the new next node... not confusing at all
                return prevNodePtr; 
            }
            struct Node corruptedNode = *(corrputedNodePtr);

            // attempt to read corrupted node metadata
            if (get_node_prev(corrputedNodePtr) == nodePtr){
                // we can trust the prev ptr of the corrupted node
                // n.next = n.next2 = n.next3 = corrputedNodePtr;
                corruptedNode.prev = corruptedNode.prev2 = corruptedNode.prev3 = nodePtr;
                struct Node newNextNode = *(prevNodePtr);
                newNextNode.prev = newNextNode.prev2 = newNextNode.prev3 = corrputedNodePtr;
// note: if there are multiple nodes between the corrupted pointers we lose them
                do {
                    *(prevNodePtr) = newNextNode;
                } while (!check_node(prevNodePtr, newNextNode));
                do {
                    *(nodePtr) = n;
                } while (!check_node(nodePtr, n));
                do {
                    *(corrputedNodePtr) = corruptedNode;
                } while (!check_node(corrputedNodePtr, corruptedNode));
                return corrputedNodePtr;
            } else {
                // cannot trust corrupted node at all
                // overwrite corrupted area with new data
                struct Node newNode;
                struct Node* newNodePtr = corrputedNodePtr;
                newNode.state = FREE;
                // prevNodePtr - nodeptr - size(nodeptr) - size of node metadata - size of newnode metadata
                newNode.size = newNode.size2 = newNode.size3 = (
                    (size_t)((prevNodePtr) - (nodePtr + get_node_size(nodePtr) + 2*sizeof(struct Node))));
                newNode.prev = newNode.prev2 = newNode.prev3 = nodePtr;
                // newNode.next = newNode.next2 = newNode.next3 = prevNodePtr;
                // newNode.data = newNode.data2 = newNode.data3 = (void *)(
                //     (uint8_t *)newNodePtr + sizeof(struct Node));
                
                // n.next = n.next2 = n.next3 = newNodePtr;
                struct Node newNextNode = *(prevNodePtr);
                newNextNode.prev = newNextNode.prev2 = newNextNode.prev3 = newNodePtr;
                do {
                    *(nodePtr) = n;
                } while (!check_node(nodePtr, n));
                do {
                    *(newNodePtr) = newNode;
                } while (!check_node(newNodePtr, newNode));
                do {
                    *(prevNodePtr) = newNextNode;
                } while (!check_node(prevNodePtr, newNextNode));
                return newNodePtr;
            }
        }
        if (currentNodePtr == headPtr){
            // this would mean our current node doesnt exist
            // return assptr bc idk
            printf("\nCOULD NOT FIX PTR, VERY VERY BAD");
            // n.next = n.next2 = n.next3 = assPtr;
            do {
                *(nodePtr) = n;
            } while (!check_node(nodePtr, n));
            return assPtr;
        } else {
            // we found the correct node by working backwards
            // fix broken pointer
            // n.next = n.next2 = n.next3 = currentNodePtr;
            do {
                *(nodePtr) = n;
            } while (!check_node(nodePtr, n));
            return currentNodePtr;
        }
    } else if (direction == 1){
        // direction = 1:
        // Corrupted prev ptr: try working forwards from head
        // to get to nodePtr and repair
        struct Node* currentNodePtr = headPtr;
        struct Node* prevNodePtr = NULL;
        while ((get_node_next(currentNodePtr) != nodePtr) && (currentNodePtr != assPtr) && (is_valid_pointer(currentNodePtr, 1))){
            prevNodePtr = currentNodePtr;
            currentNodePtr = get_node_next(currentNodePtr);
        }
        struct Node n = *(nodePtr);

        if (!is_valid_pointer(currentNodePtr, 1)) {
            struct Node newPrevNode = *(prevNodePtr);

            // Look where the corrupted node should be
            struct Node* corrputedNodePtr = (struct Node*)(prevNodePtr + sizeof(struct Node) + get_node_size(prevNodePtr));
            // check there is room to rebuild corrupted node

            if ((size_t)((corrputedNodePtr) - (prevNodePtr)) < sizeof(struct Node)){
                // not enough room to rebuild corrupted node
                // bypass corrupted area
                n.prev = n.prev2 = n.prev3 = prevNodePtr;
                // newPrevNode.next = newPrevNode.next2 = newPrevNode.next3 = nodePtr;
                do {
                    *(nodePtr) = n;
                } while (!check_node(nodePtr, n));
                do {
                    *(prevNodePtr) = newPrevNode;
                } while (!check_node(prevNodePtr, newPrevNode));
                return prevNodePtr; 
            }
            struct Node corruptedNode = *(corrputedNodePtr);

            // attempt to read corrupted node metadata
            if (get_node_next(corrputedNodePtr) == nodePtr){
                // we can trust the next ptr of the corrupted node
                n.prev = n.prev2 = n.prev3 = corrputedNodePtr;
                // corruptedNode.next = corruptedNode.next2 = corruptedNode.next3 = nodePtr;
                //struct Node newPrevNode = *(prevNodePtr);
                // newPrevNode.next = newPrevNode.next2 = newPrevNode.next3 = corrputedNodePtr;
// note: if there are multiple nodes between the corrupted pointers we lose them
                do {
                    *(prevNodePtr) = newPrevNode;
                } while (!check_node(prevNodePtr, newPrevNode));
                do {
                    *(nodePtr) = n;
                } while (!check_node(nodePtr, n));
                do {
                    *(corrputedNodePtr) = corruptedNode;
                } while (!check_node(corrputedNodePtr, corruptedNode));
                return corrputedNodePtr;
            } else {
                // cannot trust corrupted node at all
                // overwrite corrupted area with new data
                struct Node newNode;
                struct Node* newNodePtr = corrputedNodePtr;
                newNode.state = FREE;
                // nodeptr - prevnodeptr - size(prevnodeptr) - size of node metadata - size of newnode metadata
                newNode.size = newNode.size2 = newNode.size3 = (
                    (size_t)((nodePtr) - (prevNodePtr + get_node_size(prevNodePtr) + 2*sizeof(struct Node))));
                newNode.prev = newNode.prev2 = newNode.prev3 = prevNodePtr;
                // newNode.next = newNode.next2 = newNode.next3 = nodePtr;
                // newNode.data = newNode.data2 = newNode.data3 = (void *)(
                //     (uint8_t *)newNodePtr + sizeof(struct Node));
                
                n.prev = n.prev2 = n.prev3 = newNodePtr;
                // newPrevNode.next = newPrevNode.next2 = newPrevNode.next3 = newNodePtr;
                do {
                    *(nodePtr) = n;
                } while (!check_node(nodePtr, n));
                do {
                    *(newNodePtr) = newNode;
                } while (!check_node(newNodePtr, newNode));
                do {
                    *(prevNodePtr) = newPrevNode;
                } while (!check_node(prevNodePtr, newPrevNode));
                return newNodePtr;
            }
        }
        if (currentNodePtr == headPtr){
            // this would mean our current node doesnt exist
            // return headPtr bc idk
            printf("\nCOULD NOT FIX PTR, VERY VERY BAD");
            n.prev = n.prev2 = n.prev3 = headPtr;
            do {
                *(nodePtr) = n;
            } while (!check_node(nodePtr, n));
            return headPtr;
        } else {
            // we found the correct node by working forwards
            // fix broken pointer
            n.prev = n.prev2 = n.prev3 = currentNodePtr;
            do {
                *(nodePtr) = n;
            } while (!check_node(nodePtr, n));
            return currentNodePtr;
        }
    }
}

void* get_node_data(struct Node* nodePtr) {
    // struct Node n = *(nodePtr);
    // uintptr_t data1 = (uintptr_t) n.data;
    // uintptr_t data2 = (uintptr_t) n.data2;
    // uintptr_t data3 = (uintptr_t) n.data3;
    // uintptr_t correctData = (data1 & data2) | (data1 & data3) | (data2 & data3);

    // n.data = n.data2 = n.data3 = (void*) correctData;
    // *(nodePtr) = n;
    void* correctData = (void*)((uint8_t*)nodePtr + sizeof(struct Node));
    return correctData;
}

uint8_t get_node_state(struct Node* nodePtr) {
    struct Node n = *(nodePtr);
    uint8_t state = n.state;
    uint8_t count = 0;
    for (int i = 0; i < 8; i++) {
        count += (state >> i) & 1;
    }
    if (count >= 4) {
        n.state = UNFREE;
    } else {
        n.state = FREE;
    }
    *(nodePtr) = n;
    return n.state;
}







int get_checksum(uint8_t* dataPtr, size_t size) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += *(dataPtr + i);
    }
    return checksum;
}





// Initialize the allocator over a provided memory block.
// Returns 0 on success, non-zero on failure.
int mm_init(uint8_t *heap, size_t heap_size) {
    // initialise head and ass senitnels
    head.size = head.size2 = head.size3 = 0;
    head.state = UNFREE;
    head.prev = head.prev2 = head.prev3 = NULL;
    // head.next = head.next2 = head.next3 = NULL;
    ass.size = ass.size2 = ass.size3 = 0;
    ass.state = UNFREE;
    ass.prev = ass.prev2 = ass.prev3 = NULL;
    // ass.next = ass.next2 = ass.next3 = NULL;

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
    struct Node heapNode = {
        (heap_size - 3*sizeof(struct Node)), FREE, NULL
    };
    heapPtr = heap;
    heapNode.size = heapNode.size2 = heapNode.size3 = (
        heap_size - 3*sizeof(struct Node));

    heapNode.checksum = 0;
    // Initialise pointers to nodes on the heap

    // put the head at the start of the heap
    struct Node *heapHeadPtr = (struct Node *) heap;

    // put the ass  at the end of the heap
    struct Node *heapAssPtr = (struct Node *)(
        (uint8_t *)heap + heap_size - sizeof(struct Node));

    // put heapNode in the middle
    struct Node *heapContentsPtr = (struct Node *)(
        (uint8_t *)heap + sizeof(head));

    // set the prev and next pointers for each node
    // head.next = head.next2 = head.next3 = heapContentsPtr;
    ass.prev = ass.prev2 = ass.prev3 = heapContentsPtr;
    heapNode.prev = heapNode.prev2 = heapNode.prev3 = heapHeadPtr;
    // heapNode.next = heapNode.next2 = heapNode.next3 = heapAssPtr;
    // set data pointer to point to where the actual data starts
    // heapNode.data = heapNode.data2 = heapNode.data3 = (void *)(
    //     (uint8_t *)heapContentsPtr + sizeof(heapNode));

    // write nodes to the heap
    do {
        *heapHeadPtr = head;
    } while (!check_node(heapHeadPtr, head));

    do {
        *heapAssPtr = ass;
    } while (!check_node(heapAssPtr, ass));

    do {
        *heapContentsPtr = heapNode;
    } while (!check_node(heapContentsPtr, heapNode));

    //  write head and ass pointers to global vars
    headPtr = heapHeadPtr;
    assPtr = heapAssPtr;
    printf("\nSIZE OF NODE: %zu", sizeof(struct Node));
    printf("\nSIZE OF HEAPNODE: %zu", sizeof(heapNode));
    // printf("\nSIZE OF HEAP: %zu", head.next->size);
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
    newNode.state = FREE;
    *(nodePtr) = n;
    newNode.size = newNode.size2 = newNode.size3 = (
        get_node_size(nodePtr) - (size + sizeof(struct Node)));

    // update current node size
    n.size = n.size2 = n.size3 = size;
    // get the correct pointer for the new node
    struct Node* newNodePtr = (struct Node*)(
        (uint8_t*)nodePtr + size + sizeof(struct Node));

    // initialise data pointer for new node
    // newNode.data = newNode.data2 = newNode.data3 = (void *)(
    //     (uint8_t *)newNodePtr + sizeof(struct Node));

    // printf("\nNEW NODE DATA PTR: %p", newNode.data);
    // get next node
    struct Node* nextNodePtr = (struct Node *) get_node_next(nodePtr);

    struct Node nextNode = *nextNodePtr;
    // update prev and next pointers
    newNode.prev = newNode.prev2 = newNode.prev3 = nodePtr;
    // newNode.next = newNode.next2 = newNode.next3 = nextNodePtr;
    // n.next = n.next2 = n.next3 = newNodePtr;
    nextNode.prev = nextNode.prev2 = nextNode.prev3 = newNodePtr;
    // write to heap
    do {
        *nodePtr = n;
    } while (!check_node(nodePtr, n));
    do {
        *newNodePtr = newNode;
    } while (!check_node(newNodePtr, newNode));
    do {
        *nextNodePtr = nextNode;
    } while (!check_node(nextNodePtr, nextNode));
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
        if (
            get_node_state(currentNodePtr) == FREE
            && get_node_size(currentNodePtr) >= size
        ) {
            // resize and allocate this node
            printf("\nMEMORY BLOCK FOUND");
            currentNode.state = UNFREE;  // unfree the node
            do {
                *currentNodePtr = currentNode;  // write back to heap
            } while (!check_node(currentNodePtr, currentNode));

            if (
                get_node_size(currentNodePtr)
                > aligned_size + sizeof(struct Node)
            ) {
                // resize node so there is some heap left for everything else
                // use aligned size here so new node is aligned
                printf("\nNODE RESIZED");
                resize_node(currentNodePtr, aligned_size);
            }
            currentNode = *currentNodePtr;
            // reset the size to the requested size to ensure API conformity
            currentNode.size = currentNode.size2 = currentNode.size3 = size;

            memset(get_node_data(currentNodePtr), 0, currentNode.size);
            found = 1;
            currentNode.checksum = 0;
            do {
                *currentNodePtr = currentNode;  // write back to heap
            } while (!check_node(currentNodePtr, currentNode));
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




// Safely read data from an allocated block starting at offset bytes into buf
// Returns the num of bytes read, or -1 if corruption or invalid ptr detected
int mm_read(void *ptr, size_t offset, void *buf, size_t len) {
    ptr = (void*)((uint8_t*)ptr - sizeof(struct Node));  // data ptr to node ptr
    // return if pointer invalid
    if (((uint8_t*)ptr) < heapPtr || ((uint8_t*)ptr) > (heapPtr + heapSize)) {
        printf("\nINVALID POINTER IN MM_READ");
        return -1;
    }
    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;

    // check for data corruption
    // i hate cpplint
    if (
        get_checksum(
            get_node_data(nodePtr),
            get_node_size(nodePtr)
        )
        != n.checksum
    ) {
        printf("\nDATA CORRUPTION DETECTED IN MM_READ");
        return -1;
    }



    uint8_t* dataPtr = (uint8_t*)get_node_data(nodePtr);
    uint8_t* buff = (uint8_t*)buf;
    size_t size = get_node_size(nodePtr);
    // nuh uh if block is free or doesnt exist
    if (get_node_state(nodePtr) != UNFREE) {
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

    for (size_t i = 0; i < len; i++) {
        do {
            *(buff+i) = *(dataPtr+i);
        } while (*(buff+i) != *(dataPtr+i));

        // printf("\n%d", *(buff+i));
        numBytes++;
    }
    return numBytes;
}




// Safely write data into an allocated block starting at offset bytes from src.
// Returns the num of bytes written, or -1 if corruption or invalid ptr detected
int mm_write(void *ptr, size_t offset, const void *src, size_t len) {
    ptr = (void*)((uint8_t*)ptr - sizeof(struct Node));  // data ptr to node ptr

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

    // apply offset
    dataPtr += offset;
    for (size_t i = 0; i < (len); i++) {
        // ensure each byte is written correctly
        do {
            *(dataPtr+i) = *(source+i);
        } while ( *(dataPtr+i) != *(source+i) );

        numBytes++;
    }

    uint16_t checksum = get_checksum((uint8_t*)src, len);
    n.checksum = checksum;
    do {
        *nodePtr = n;
    } while (!check_node(nodePtr, n));
    return numBytes;
}




// updates the pointers of previous and next nodes to "delete" the node.
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
    // prevNode.next = prevNode.next2 = prevNode.next3 = nextNodePtr;
    // write to heap
    do {
        *prevNodePtr = prevNode;
    } while (!check_node(prevNodePtr, prevNode));
    do {
        *nextNodePtr = nextNode;
    } while (!check_node(nextNodePtr, nextNode));
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
        uint8_t byte = pattern[(i + x) % 5];
        do {
            dataPtr[i] = byte;
        } while ( dataPtr[i] != byte );
    }
    return;
}




void merge_node(struct Node* nodePtr) {
    struct Node* currentNodePtr = nodePtr;
    struct Node currentNode = *currentNodePtr;
    struct Node* nextNodePtr = get_node_next(currentNodePtr);
    struct Node nextNode = *nextNodePtr;
    struct Node* prevNodePtr = get_node_prev(currentNodePtr);
    struct Node prevNode = *prevNodePtr;
    if (
        (get_node_state(prevNodePtr) == FREE)
        && (get_node_state(nextNodePtr) == FREE)
        && (get_node_prev(prevNodePtr) != NULL)
        && (get_node_next(nextNodePtr) != NULL)
    ) {
        struct Node currentNode = *currentNodePtr;
        struct Node nextNode = *nextNodePtr;
        struct Node prevNode = *prevNodePtr;

        // get new size of merged node
        size_t newSize = (size_t)(
            (uint8_t*)get_node_next(nextNodePtr)
            - (uint8_t*)get_node_data(prevNodePtr));
        printf("\nB1: NEW SIZE AFTER MERGE: %zu", newSize);
        // "delete" redundant nodes (just update prev and next ptrs)
        delete_node(currentNodePtr);
        delete_node(nextNodePtr);
        prevNode = *prevNodePtr;
        // update size
        prevNode.size = prevNode.size2 = prevNode.size3 = newSize;
        // update heap
        do {
            *prevNodePtr = prevNode;
        } while (!check_node(prevNodePtr, prevNode));
        // fill with 5B pattern
        overwrite_data(get_node_data(prevNodePtr), newSize);

    } else if (get_node_state(prevNodePtr) == FREE) {
        // get new size of merged node
        size_t newSize = (size_t)(
            (uint8_t*)get_node_next(currentNodePtr)
            - (uint8_t*)get_node_data(prevNodePtr));

        // "delete" redundant node (just update prev and next ptrs)
        delete_node(currentNodePtr);
        // update the prev and next ptrs from delete_node()
        prevNode = *prevNodePtr;
        // update size
        prevNode.size = prevNode.size2 = prevNode.size3 = newSize;
        // updates heap
        do {
            *prevNodePtr = prevNode;
        } while (!check_node(prevNodePtr, prevNode));
        // fill with 5B pattern
        overwrite_data(get_node_data(prevNodePtr), newSize);
    } else if (get_node_state(nextNodePtr) == FREE) {
        size_t newSize = (size_t)(
            (uint8_t*)get_node_next(nextNodePtr)
            - (uint8_t*)get_node_data(currentNodePtr));

        delete_node(nextNodePtr);
        currentNode = *currentNodePtr;
        currentNode.size = currentNode.size2 = currentNode.size3 = newSize;
        do {
            *currentNodePtr = currentNode;
        } while (!check_node(currentNodePtr, currentNode));
        overwrite_data(get_node_data(currentNodePtr), newSize);
    } else {
        // if no merging to be done, just overwrite with 5B pattern
        overwrite_data(
            get_node_data(currentNodePtr), get_node_size(currentNodePtr));
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

    // return if prev or next pointers are invalid
    printf("\n%p", get_node_prev(nodePtr));
    printf("\n%p", get_node_next(nodePtr));
    
    if (
        !is_valid_pointer(get_node_prev(nodePtr), 1)
        ||
        !is_valid_pointer(get_node_next(nodePtr), 1)
    ) {
        printf("\nINVALID PREV/NEXT POINTER");
        return;
    }
    
    // check for double free
    if (get_node_state(nodePtr) != UNFREE) {
        printf("\nCANNOT FREE NON-UNFREE NODE");
        return;
    }
    struct Node n = *nodePtr;
    // get size and data pointer of node
    size_t size = n.size;
    //uint8_t* dataPtr = n.data;
    // free node BEFORE calling merge_node() - IMPORTANT
    n.state = FREE;
    do {
        *nodePtr = n;
    } while (!check_node(nodePtr, n));

    // Merge with adjacent frees and overwrite data with 5B pattern
    merge_node(nodePtr);

    printf("\nSUCCESSFULLY FREED BLOCK OF SIZE %zu", size);
    return;
}
