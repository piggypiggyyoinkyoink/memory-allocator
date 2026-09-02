# Memory Allocator
Memory allocator written in C, implementing basic allocator functions:  
- `int mm_init(uint8_t *heap, size_t heap_size)`: Initialises the heap used by the allocator. `heap` is the pointer to the heap and `heap_size` is the size of the heap in bytes. Returns 0 on success, -1 otherwise.  
- `void* mm_malloc(size_t size)`: allocates a block of memory from the heap with 40-byte aligned payload, returning the pointer to this block if successful or `NULL` on failure. `size_t` is the size of the payload of the allocated block.  
- `void mm_free(void* ptr)`: Frees the allocated block of memory at `ptr`. If `ptr` points to an already freed block of memory, or an invalid memory location, does nothing.  
- `void* mm_realloc(void *ptr, size_t new_size)`: Reallocates the block of memory referenced by `ptr` to size `new_size`, returning the new pointer of the resized block. Returns `NULL` on failure.
- `int mm_read(void *ptr, size_t offset, void *buf, size_t len)`: Reads `len` bytes from the allocated block referenced by `ptr`, starting at `offset` bytes, into `buf`, returning the num of bytes read, or -1 if corruption or invalid ptr detected.
- `int mm_write(void *ptr, size_t offset, const void *src, size_t len)`: Writes `len` bytes into the allocated block referenced by `ptr`, starting at `offset` bytes, from `src`, returning the num of bytes written, or -1 if corruption or invalid ptr detected.

## Specifications
This code was designed to meet the following specifications:  
- All pointers to memory blocks must be 40-byte aligned (with respect to the start of the heap)  .
- Should be robust and withstand a large number of allocations/frees.
- Should handle errors without crashing.
- All non-allocated code should match a repeating 5-byte pattern, set before passing the heap pointer to `mm_init`.
- Whenever a block is freed, its payload should be set to the same repeating pattern.
- It is assumed that `offset` + `len` = the size of block `ptr` in `mm_write`. Partial writes are treated as errors.
- The allocator must be resistant to block or heap metadata corruption.
- Block payload corruption must be detected or corrected.
  
