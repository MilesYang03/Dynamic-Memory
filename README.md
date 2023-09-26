# How it works
A free list is initialized as a singular large block of memory. The free list is designed as a doubly-linked linked list, and each chunk of memory has a header that is the class metadata_t. This header contains 3 important pieces of information: the size of the chunk of memory, the previous pointer, and the next pointer. The dmalloc() and dfree() functions simulate dynamic memory allocation and freeing.

# dmalloc()
Dynamic allocation takes 1 argument: the size of the allocation. Using the first-fit allocation policy implemented in the findFirstFit() function, the first block of memory in the free list is allocated for the user. The block is unlinked from the doubly-linked free list and is returned to the function caller.

# dfree()
Dynamic freeing takes 1 argument: the pointer of the block of allocated memory. Because the free list has been designed to be sorted by address, we can insert the allocated memory block back into the free list easily by linearly searching through the free list. The size of the chunk of memory is stored in the header, so we can insert the chunk of memory with the correct size.

# Coalescing
Another important task involved in dynamic memory allocation is coalescing. As we allocate and de-allocate chunks of memory, the free list can become long and fragmented with small chunks of adjacent memory blocks. By coalescing these adjacent memory blocks, we can store larger chunks of free memory, and can in the future keep allocating larger chunks of data.
