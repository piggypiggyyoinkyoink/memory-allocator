#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iso646.h>
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

    printf("\nSIZE OF NODE: %d",sizeof(heapNode));
    
    
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

    printf("\nSIZE: %d",head.next->size);
    return 0;
};


void resize_node(struct Node* nodePtr, size_t size){
    //initialise current node as n
    struct Node n = *(nodePtr);
    if (n.size < size){
        printf("\nOH FUCK");
    }
    //initialise new node
    struct Node newNode;
    newNode.state = FREE;
    newNode.size = (n.size - (size + sizeof(struct Node)));
    //update current node size
    n.size = size;
    //get the correct pointer for the new node
    struct Node* newNodePtr = (struct Node*)((uint8_t*)nodePtr + size + sizeof(struct Node)); //wtf is this pointer arithmetic
    //initialise data pointer for new node
    newNode.data = (void *)((uint8_t *)newNodePtr + sizeof(struct Node));
    //get next node
    struct Node* nextNodePtr = (struct Node *) n.next;
    struct Node nextNode = *nextNodePtr;
    //update prev and next pointers
    newNode.prev = nodePtr;
    newNode.next = nextNodePtr;
    n.next = newNodePtr;
    nextNode.prev = newNodePtr;
    //write to heap
    *nodePtr = n;
    *newNodePtr = newNode;
    *nextNodePtr = nextNode;
}

// Allocate a block with ALIGN-byte aligned payload. Returns
// NULL on failure.
void *mm_malloc(size_t size){
    printf("\nSIZE_MALLOC: %d", size);
    uint8_t end = 0;
    uint8_t found = 0;
    //start at the first data node
    struct Node* currentNodePtr = (struct Node *) head.next;
    struct Node currentNode= *currentNodePtr;
    //check all nodes until we find a free one of sufficient size, or reach ass
    while ((not end) and (not found)){
        if (currentNode.state == FREE and currentNode.size >= size){
            //resize and allocate this node
            printf("\nLETS FUCKING GO");
            currentNode.state = UNFREE; //unfree the node
            *currentNodePtr = currentNode;//write back to heap
            if (currentNode.size > size ){
                resize_node(currentNodePtr, size);//resize node so there is some heap left for everything else
            }
            found = 1;
            return currentNodePtr;
        }else if (currentNode.next == NULL){
            //no available nodes of sufficient size
            end = 1;
            printf("\nNOPE");
            return NULL;
        }else{
            //try next node
            currentNodePtr = (struct Node *) currentNode.next;
            currentNode = *currentNodePtr;
            printf("\nNEXT");
        }
    }
    printf("Hello World");
    return NULL;
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

//doesnt actually delete the node but updates the pointers of previous and next nodes so that logically it ceases to exist.
void delete_node(struct Node* nodePtr){
    struct Node* currentNodePtr = nodePtr;
    struct Node currentNode = *nodePtr;
    //dont delete non-free nodes
    if (currentNode.state != FREE){
        return;
    }
    //get prev and next nodes
    struct Node* nextNodePtr = currentNode.next;
    struct Node nextNode = *nextNodePtr;
    struct Node* prevNodePtr = currentNode.prev;
    struct Node prevNode = *prevNodePtr;
    //update pointers to bypass deleted node.
    nextNode.prev = prevNodePtr;
    prevNode.next = nextNodePtr;
    //write to heap
    *prevNodePtr = prevNode;
    *nextNodePtr = nextNode;
    return;
}

//overwrite a node's data with the 5 byte pattern
void overwrite_data(uint8_t* dataPtr, size_t size){
    //to ensure pattern is properly aligned
    int x = (dataPtr - heapPtr) % 5;
    //overwrite node data with 5 byte pattern.
    //if x = 0 -> 0,1,2,3,4; x = 1 -> 1,2,3,4,0
    for (size_t i=0; i<size-4; i+=5){//idk if i should be int or size_t here
        *(dataPtr+i) = pattern[(0+x)%5];
        *(dataPtr+i+1) = pattern[(1+x)%5];
        *(dataPtr+i+2) = pattern[(2+x)%5];
        *(dataPtr+i+3) = pattern[(3+x)%5];
        *(dataPtr+i+4) = pattern[(4+x)%5];
    }
}

void merge_node(struct Node* nodePtr){
    struct Node* currentNodePtr = nodePtr;
    struct Node currentNode = *currentNodePtr;
    struct Node* nextNodePtr = currentNode.next;
    struct Node nextNode = *nextNodePtr;
    struct Node* prevNodePtr = currentNode.prev;
    struct Node prevNode = *prevNodePtr;
    if (prevNode.state == FREE and nextNode.state == FREE){ //should probs use a separate function to check freeness that can be applied anywhere and takes bitflips into acc.
        //get new size of merged node
        size_t newSize = prevNode.size + currentNode.size + nextNode.size + 2*sizeof(struct Node);
        //"delete" redundant nodes (just update prev and next ptrs)
        delete_node(currentNodePtr);
        delete_node(nextNodePtr);
        //update size
        prevNode.size = newSize;
        //update heap
        *prevNodePtr = prevNode;
        //fill with 5B pattern
        overwrite_data(prevNode.data, newSize);

    }else if (prevNode.state == FREE){
        //get new size of merged node
        size_t newSize = prevNode.size + currentNode.size + sizeof(struct Node);
        //"delete" redundant node (just update prev and next ptrs)
        delete_node(currentNodePtr);
        //update size
        prevNode.size = newSize;
        //updates heap
        *prevNodePtr = prevNode;
        //fill with 5B pattern
        overwrite_data(prevNode.data, newSize);
    }else if (nextNode.state == FREE){
        size_t newSize = currentNode.size + nextNode.size + sizeof(struct Node);
        delete_node(nextNodePtr);
        currentNode.size = newSize;
        *currentNodePtr = currentNode;
        overwrite_data(currentNode.data, newSize);
    }else{
        //if no merging to be done, just overwrite with 5B pattern
        overwrite_data(currentNode.data, currentNode.size);
    }
    return;
}



// Free a previously-allocated pointer (ignore NULL).
// Must detect double-free.
void mm_free(void *ptr){
    //check for null pointer
    if (ptr == NULL){
        //fuck off if null
        return;
    }
    //Get the node
    struct Node* nodePtr = ptr;
    struct Node n = *nodePtr;
    //check for double free
    if (n.state != UNFREE){
        printf("\nWhoopsie");
        return;
    }
    //get size and data pointer of node
    size_t size = n.size;
    uint8_t* dataPtr = n.data;
    //Merge with adjacent frees and overwrite data with 5B pattern
    merge_node(nodePtr);
    //free node
    n.state = FREE;
    *nodePtr = n;//write back to heap

    
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

    void* ptr = mm_malloc(76);
    void* ptr2 = mm_malloc(76);
    void* ptr3 = mm_malloc(700);
    mm_free(ptr);
    mm_free(ptr2);
    void* ptr4 = mm_malloc(100);

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