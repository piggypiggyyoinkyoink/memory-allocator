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

// define metadata nodes in singly linked list
struct Node{
    size_t size;  // size of memory block
    uint8_t state;  // free or not free
    size_t size2;  // size of memory block
    size_t size3;  // size of memory block
    uint16_t checksum;  // checksum
};


static struct Node* headPtr;
static struct Node* endPtr;
static uint8_t *heapPtr;
static size_t heapSize;
static uint8_t pattern[5];


// check a pointer is within the bounds of the heap
int is_valid_pointer(void* ptr, int sentinels_valid) {
    // sentinel pointers are valid
    if (sentinels_valid) {
        if (((uint8_t*)ptr) < (heapPtr)
        || ((uint8_t*)ptr) > (heapPtr + heapSize-sizeof(struct Node))
        ) {
            printf("\nINVALID POINTER nsi");
            return 0;
        } else {
            return 1;
        }
    // sentinel pointers not valid (so cant free sentinels by accident)
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
        || (n.state != node.state)
        || (n.checksum != node.checksum)
    ) {
        return 0;
    } else {
        return 1;
    }
}



// get size of a node, correcting any discrepancies
size_t get_node_size(struct Node* nodePtr) {
    struct Node n = *(nodePtr);
    size_t correctSize;
    correctSize = (n.size & n.size2) | (n.size & n.size3) | (n.size2 & n.size3);
    n.size = n.size2 = n.size3 = correctSize;
    *(nodePtr) = n;
    return correctSize;
}



// use node size to caluclate address of next node
struct Node* get_node_next(struct Node* nodePtr) {
    struct Node n = *(nodePtr);
    if ((uintptr_t)nodePtr >= ((uintptr_t)endPtr - sizeof(struct Node))) {
        return NULL;
    }
    // no alignment padding
    if (get_node_size(nodePtr)%40 == 0) {
        size_t tmp = get_node_size(nodePtr);
        struct Node* correctNext = (struct Node*)(
            (uint8_t*)nodePtr + sizeof(struct Node) + tmp);
        return correctNext;
    }
    // add alignment padding to get correct next ptr
    size_t tmp = get_node_size(nodePtr) + (40 - get_node_size(nodePtr)%40);
    struct Node* correctNext = (struct Node*)(
        (uint8_t*)nodePtr + sizeof(struct Node) + tmp);
    if (!is_valid_pointer((void*)correctNext, 1)) {
        // this should never run but may prevent segfault if data mega corrupted
        return endPtr;
    }
    return correctNext;
}



// get previous node ptr by iterating from head until next node is current node
struct Node* get_node_prev(struct Node* nodePtr) {
    struct Node* currentNodePtr = headPtr;
    while ((get_node_next(currentNodePtr) != nodePtr)
    && (currentNodePtr != endPtr)
    && (is_valid_pointer(currentNodePtr, 1))) {
        currentNodePtr = get_node_next(currentNodePtr);
    }
    return currentNodePtr;
}



// get ptr to node payload data
void* get_node_data(struct Node* nodePtr) {
    void* correctData = (void*)((uint8_t*)nodePtr + sizeof(struct Node));
    return correctData;
}



// determine if node is free or unfree, correcting any discrepancies
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



// calculate checksum of data block
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
    // initialise head and end senitnels
    struct Node head;
    head.size = head.size2 = head.size3 = 0;
    head.state = UNFREE;
    struct Node end;
    end.size = end.size2 = end.size3 = 0;
    end.state = UNFREE;

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
        (heap_size - 3*sizeof(struct Node)), FREE
    };
    heapPtr = heap;
    heapNode.size = heapNode.size2 = heapNode.size3 = (
        heap_size - 3*sizeof(struct Node));

    heapNode.checksum = 0;

    // put the head at the start of the heap
    struct Node *heapHeadPtr = (struct Node *) heap;

    // put the end  at the end of the heap
    struct Node *heapEndPtr = (struct Node *)(
        (uint8_t *)heap + heap_size - sizeof(struct Node));

    // put heapNode in the middle
    struct Node *heapContentsPtr = (struct Node *)(
        (uint8_t *)heap + sizeof(head));


    // write nodes to the heap
    do {
        *heapHeadPtr = head;
    } while (!check_node(heapHeadPtr, head));

    do {
        *heapEndPtr = end;
    } while (!check_node(heapEndPtr, end));

    do {
        *heapContentsPtr = heapNode;
    } while (!check_node(heapContentsPtr, heapNode));

    //  write head and end pointers to global vars
    headPtr = heapHeadPtr;
    endPtr = heapEndPtr;
    printf("\nSIZE OF NODE: %zu", sizeof(struct Node));
    printf("\nSIZE OF HEAPNODE: %zu", sizeof(heapNode));
    return 0;
}




void resize_node(struct Node* nodePtr, size_t size) {
    // initialise current node as n
    struct Node n = *(nodePtr);

    if (get_node_size(nodePtr) < size) {
        // this should never run since resize_node is only called
        // when size > node size + metadata size
        return;
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

    do {
        *nodePtr = n;
    } while (!check_node(nodePtr, n));
    do {
        *newNodePtr = newNode;
    } while (!check_node(newNodePtr, newNode));
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
    // check all nodes until we find a free one of sufficient size, or reach end
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
            // removing this memset somehow messes up the checksum
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
    // return -1 if block is free or doesnt exist
    if (get_node_state(nodePtr) != UNFREE) {
        printf("CANNOT WRITE TO FREE BLOCK");
        return -1;
    }
    // return -1 if try to read too much
    if (len > (size-offset)) {
        printf("READ GOES OUT OF BOUNDS");
        return -1;
    }

    int numBytes = 0;
    // apply offset
    dataPtr += offset;

    // read data 1 byte at a time
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

    int numBytes = 0;

    // apply offset
    dataPtr += offset;
    // write data 1 byte at a time
    for (size_t i = 0; i < (len); i++) {
        // ensure each byte is written correctly
        do {
            *(dataPtr+i) = *(source+i);
        } while ( *(dataPtr+i) != *(source+i) );
        numBytes++;
    }

    // if partial write occurs
    if (len < (size-offset)) {
        // pad remaining bytes with zeros
        memset(dataPtr + offset + len, 0, size - (offset + len));
    }

    // update checksum
    uint16_t checksum = get_checksum((uint8_t*)src, len);
    n.checksum = checksum;

    // write to heap
    do {
        *nodePtr = n;
    } while (!check_node(nodePtr, n));
    return numBytes;
}




// overwrite a node's data with the 5 byte pattern
void overwrite_data(uint8_t* dataPtr, size_t size) {
    if (size == 0) {
        // this shouldnt ever run
        return;
    }
    // to ensure pattern is properly aligned
    int x = ((intptr_t)dataPtr - (intptr_t)heapPtr) % 5;
    // write pattern 1 byte at a time
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
    // previous and next nodes free -> merge all 3 into previous
    if (
        (get_node_state(prevNodePtr) == FREE)
        && (get_node_state(nextNodePtr) == FREE)
    ) {
        struct Node currentNode = *currentNodePtr;
        struct Node nextNode = *nextNodePtr;
        struct Node prevNode = *prevNodePtr;

        // get new size of merged node
        size_t newSize = (size_t)(
            (uint8_t*)get_node_next(nextNodePtr)
            - (uint8_t*)get_node_data(prevNodePtr));
        // update node
        prevNode = *prevNodePtr;
        // update size
        prevNode.size = prevNode.size2 = prevNode.size3 = newSize;
        // update heap
        do {
            *prevNodePtr = prevNode;
        } while (!check_node(prevNodePtr, prevNode));
        // fill with 5B pattern
        overwrite_data(get_node_data(prevNodePtr), newSize);
    // only previous node free -> merge both into previous
    } else if (get_node_state(prevNodePtr) == FREE) {
        // get new size of merged node
        size_t newSize = (size_t)(
            (uint8_t*)get_node_next(currentNodePtr)
            - (uint8_t*)get_node_data(prevNodePtr));

        prevNode = *prevNodePtr;
        // update size
        prevNode.size = prevNode.size2 = prevNode.size3 = newSize;
        // updates heap
        do {
            *prevNodePtr = prevNode;
        } while (!check_node(prevNodePtr, prevNode));
        // fill with 5B pattern
        overwrite_data(get_node_data(prevNodePtr), newSize);
    // only next node free -> merge both into current
    } else if (get_node_state(nextNodePtr) == FREE) {
        size_t newSize = (size_t)(
            (uint8_t*)get_node_next(nextNodePtr)
            - (uint8_t*)get_node_data(currentNodePtr));

        currentNode = *currentNodePtr;
        currentNode.size = currentNode.size2 = currentNode.size3 = newSize;
        do {
            *currentNodePtr = currentNode;
        } while (!check_node(currentNodePtr, currentNode));
        // fill with 5B pattern
        overwrite_data(get_node_data(currentNodePtr), newSize);
    // neither adjacent node free -> just overwrite current with 5B pattern
    } else {
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

    printf("\n%p", get_node_prev(nodePtr));
    printf("\n%p", get_node_next(nodePtr));

    // return if prev or next pointers are invalid
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
    // get size of node
    size_t size = n.size;
    // free node BEFORE calling merge_node() - IMPORTANT
    n.state = FREE;
    // write to heap
    do {
        *nodePtr = n;
    } while (!check_node(nodePtr, n));

    // Merge with adjacent frees and overwrite data with 5B pattern
    merge_node(nodePtr);

    printf("\nSUCCESSFULLY FREED BLOCK OF SIZE %zu", size);
    return;
}

// Resize a previously allocated block to new_size bytes,
// preserving data. [See additional credit]
void *mm_realloc(void *ptr, size_t new_size) {
    printf("\nREALLOCATING BLOCK TO SIZE %zu", new_size);

    // check for null or invalid pointer
    if (ptr == NULL || !is_valid_pointer(ptr, 0)) {
        printf("\nINVALID POINTER IN MM_REALLOC");
        return NULL;
    }

    // check for size 0
    if (new_size == 0) {
        printf("\nCANNOT REALLOC TO SIZE 0");
        return NULL;
    }

    ptr = (void*)((uint8_t*)ptr - sizeof(struct Node));  // data ptr to node ptr

    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;
    size_t old_size = get_node_size(nodePtr);
    if (get_node_state(nodePtr) != UNFREE) {
        // cannot realloc a free block
        printf("\nCANNOT REALLOC A FREE BLOCK");
        return NULL;
    }

    if (old_size == new_size) {
        // no resizing needed
        printf("\nREALLOC: NODE UNCHANGED");
        return get_node_data(nodePtr);
    } else if (new_size < old_size) {
        // node needs shrinking
        if (
            (old_size - new_size)
            > sizeof(struct Node)
        ) {
            // resize node to smaller size if sufficient space
            size_t aligned_size;
            if (new_size %40 != 0) {
                aligned_size = new_size + (40 - (new_size%40));
            } else {
                aligned_size = new_size;
            }
            // use aligned size to conform to 40 byte alignment
            resize_node(nodePtr, aligned_size);
            n = *nodePtr;  // update n after resizing
            // reset to non-aligned size
            n.size = n.size2 = n.size3 = new_size;
            n.checksum = get_checksum(
                (uint8_t*)get_node_data(nodePtr), new_size);
            // rewrite to heap
            do {
                *nodePtr = n;
            } while (!check_node(nodePtr, n));
            printf("\nREALLOC: NODE SHRUNK");
            return get_node_data(nodePtr);

        } else {
            // not enough room to create a new node, leave node unchanged
            printf("\nREALLOC: NODE UNCHANGED");
            return get_node_data(nodePtr);
        }
    } else {
        // node needs expanding
        struct Node* nextNodePtr = get_node_next(nodePtr);
        struct Node nextNode = *nextNodePtr;
        struct Node* prevNodePtr = get_node_prev(nodePtr);
        struct Node prevNode = *prevNodePtr;
        // check if prev and next nodes are free and have enough space
        if (
            (get_node_state(prevNodePtr) == FREE)
            && (get_node_state(nextNodePtr) == FREE)
            && ((old_size + 2* sizeof(struct Node) + get_node_size(nextNodePtr)
            + get_node_size(prevNodePtr)) >= new_size)
        ) {
            // merge current, prev and next nodes into one supernode :)
            size_t combined_size =
                old_size + 2*sizeof(struct Node)
                + get_node_size(nextNodePtr)
                + get_node_size(prevNodePtr);
            size_t aligned_size;
            prevNode.size = prevNode.size2 = prevNode.size3 = combined_size;
            do {
                *prevNodePtr = prevNode;
            } while (!check_node(prevNodePtr, prevNode));
            if (combined_size - new_size > sizeof(struct Node)) {
                if (new_size %40 != 0) {
                    aligned_size = new_size + (40 - (new_size%40));
                } else {
                    aligned_size = new_size;
                }
                // resize if enough space left over
                resize_node(prevNodePtr, aligned_size);
                prevNode = *prevNodePtr;  // update n after resizing
                // reset to non-aligned size
                prevNode.size = prevNode.size2 = prevNode.size3 = new_size;
                // update checksum so mm_read doesnt crashout
                n.checksum = get_checksum(
                    (uint8_t*)get_node_data(nodePtr), old_size);
            } else {
                n.checksum = get_checksum(
                    (uint8_t*)get_node_data(nodePtr), old_size);
            }
            do {
                    *nodePtr = n;
            } while (!check_node(nodePtr, n));
            mm_read(get_node_data(nodePtr), 0,
                    get_node_data(prevNodePtr), old_size);
            prevNode.checksum = get_checksum(
                    (uint8_t*)get_node_data(prevNodePtr), new_size);
            do {
                *prevNodePtr = prevNode;
            } while (!check_node(prevNodePtr, prevNode));
            printf("\nREALLOC: NODE EXPANDED INTO PREV AND NEXT");
            return get_node_data(prevNodePtr);
        // check if next node is free and has enough space
        } else if (
            (get_node_state(nextNodePtr) == FREE)
            && ((old_size + sizeof(struct Node)
            + get_node_size(nextNodePtr)) >= new_size)
        ) {
            // merge with next node
            size_t combined_size =
                old_size + sizeof(struct Node) + get_node_size(nextNodePtr);
            size_t aligned_size;
            n.size = n.size2 = n.size3 = combined_size;
            do {
                *nodePtr = n;
            } while (!check_node(nodePtr, n));
            if (combined_size - new_size > sizeof(struct Node)) {
                if (new_size %40 != 0) {
                    aligned_size = new_size + (40 - (new_size%40));
                } else {
                    aligned_size = new_size;
                }
                // resize if enough space left over
                resize_node(nodePtr, aligned_size);
                n = *nodePtr;  // update n after resizing
                // reset to non-aligned size
                n.size = n.size2 = n.size3 = new_size;
                // update checksum so mm_read doesnt crashout
                n.checksum = get_checksum(
                    (uint8_t*)get_node_data(nodePtr), new_size);

            } else {
                n.checksum = get_checksum(
                    (uint8_t*)get_node_data(nodePtr), combined_size);
            }

            do {
                *nodePtr = n;
            } while (!check_node(nodePtr, n));
            printf("\nREALLOC: NODE EXPANDED INTO NEXT");
            return get_node_data(nodePtr);
        } else {
            // look elsewhere
            void* newPtr = mm_malloc(new_size);
            struct Node* newNodePtr = (struct Node*)(
                (uint8_t*)newPtr - sizeof(struct Node));
            struct Node newNode = *newNodePtr;
            if (newPtr == NULL) {
                // allocation failed
                return NULL;
            } else {
                // copy data to new block
                mm_read(get_node_data(nodePtr), 0,
                    newPtr, old_size);

                // free old block
                mm_free(get_node_data(nodePtr));
                // update checksum of new block
                newNode.checksum = get_checksum(
                    (uint8_t*)newPtr, new_size);
                do {
                    *newNodePtr = newNode;
                } while (!check_node(newNodePtr, newNode));
                printf("\nREALLOC: NEW BLOCK ALLOCATED");
                return newPtr;
            }
        }
    }
}
