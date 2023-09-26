#include <stdio.h>  // needed for size_t etc.
#include <unistd.h> // needed for sbrk etc.
#include <sys/mman.h> // needed for mmap
#include <assert.h> // needed for asserts
#include "dmm.h"

/* 
 * The lab handout and code guide you to a solution with a single free list containing all free
 * blocks (and only the blocks that are free) sorted by starting address.  Every block (allocated
 * or free) has a header (type metadata_t) with list pointers, but no footers are defined.
 * That solution is "simple" but inefficient.  You can improve it using the concepts from the
 * reading.
 */

/* 
 *size_t is the return type of the sizeof operator.   size_t type is large enough to represent
 * the size of the largest possible object (equivalently, the maximum virtual address).
 */

typedef struct metadata {
  size_t size;
  struct metadata* next;
  struct metadata* prev;
} metadata_t;

/*
 * Head of the freelist: pointer to the header of the first free block.
 */

static metadata_t* freelist = NULL;

metadata_t* findFirstFit(size_t numbytes) {
  metadata_t* curr = freelist;
  while (curr != NULL) {
    if (curr->size < METADATA_T_ALIGNED) {
      curr = curr->next;
      continue;
    }
    else if (curr->size >= numbytes + METADATA_T_ALIGNED) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}


void* dmalloc(size_t numbytes) {

  if(freelist == NULL) {
    if(!dmalloc_init()) {
      return NULL;
    }
  }

  assert(numbytes > 0);

  /* your code here */
  size_t numbytesaligned = ALIGN(numbytes);
  metadata_t* target = findFirstFit(numbytesaligned);
  if (target != NULL) {
    // printf("target size: %d\n", (int)target->size);
    // printf("numbytesaligned: %d\n", numbytesaligned);
    size_t rem = target->size - (numbytesaligned + METADATA_T_ALIGNED);
    if (rem >= ALIGN(1)) {
      // split the target block
      metadata_t* new_block = (metadata_t*)(target+1);
      void* ptr = ((void*)(new_block))+numbytesaligned;
      new_block = (metadata_t*) ptr;

      if (freelist == target) {
        freelist = new_block;
        new_block->size = rem;
      }
      else {
        target->prev->next = new_block;
        if (target->next != NULL) {
          target->next->prev = new_block;
        }
        new_block->next = target->next;
        new_block->prev = target->prev;
        new_block->size = rem;
      }

      target->next = NULL;
      target->prev = NULL;
      target->size = numbytesaligned;
    }
    else {
      // no need to split, just take target out
      if (freelist == target) {
        freelist = target->next;
      }
      else {
        target->prev->next = target->next;
        if (target->next != NULL) {
          target->next->prev = target->prev;
        }
      }
      target->next = NULL;
      target->prev = NULL;
      target->size = numbytesaligned;

    }
    // print_freelist();
    // printf("target pointer: %p\n", target);
    // printf("target data pointer: %p\n", target+1);
    return (void*)(target)+METADATA_T_ALIGNED;
  }

  return NULL;
}

void coallesce(metadata_t* prevptr, metadata_t* nextptr) {
  // coallesce after freeing allocated mem chunk (adding back into freelist)
  // add next to prev
  prevptr->next = nextptr->next;
  if (nextptr->next != NULL) {
    nextptr->next->prev = prevptr;
  }
  prevptr->size += METADATA_T_ALIGNED + nextptr->size;
  nextptr->size = 0;
  nextptr->next = NULL;
  nextptr->prev = NULL;
}

void dfree(void* ptr) {
  /* your code here */
  metadata_t* prev = freelist;
  metadata_t* curr = freelist;
  metadata_t* target = (metadata_t*)(ptr-METADATA_T_ALIGNED);
  while (curr != NULL) {
    prev = curr->prev;
    if (freelist == NULL || freelist->size == 0) {
      // if there is no space in freelist
      freelist = target;
      freelist->prev = NULL;
      freelist->next = NULL;
      break;
    }
    else if (curr == freelist && target < curr) {
      // if target comes before the freelist
      target->next = freelist;
      curr->prev = target;
      freelist = target;
      break;
    }
    else if (curr != freelist && prev < target && target < curr) {
      // if target falls between 2 blocks in freelist linkedlist
      prev->next = target;
      curr->prev = target;
      target->prev = prev;
      target->next = curr;
      break;
    }
    else if (curr->next == NULL && (void*)(curr)+curr->size < (void*)target) {
      // if target falls after freelist
      curr->next = target;
      target->prev = curr;
      target->next = NULL;
      break;
    }
    curr = curr->next;
  }
  metadata_t* currptr = freelist;
  while (currptr != NULL) {
    metadata_t* nextptr = (metadata_t*)(currptr+1);
    void* temp = (void*)(nextptr) + currptr->size;
    nextptr = (metadata_t*)(temp);
    if (currptr->next == nextptr) { // if curr's next block is next to curr
      coallesce(currptr, nextptr);
      currptr = freelist;
    }
    else {
      currptr = currptr->next;
    }
  }
  // print_freelist();
}

/*
 * Allocate heap_region slab with a suitable syscall.
 */
bool dmalloc_init() {

  size_t max_bytes = ALIGN(MAX_HEAP_SIZE);

  /*
   * Get a slab with mmap, and put it on the freelist as one large block, starting
   * with an empty header.
   */
  freelist = (metadata_t*)
     mmap(NULL, max_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (freelist == (void *)-1) {
    perror("dmalloc_init: mmap failed");
    return false;
  }
  freelist->next = NULL;
  freelist->prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  return true;
}


/* for debugging; can be turned off through -NDEBUG flag*/


// This code is here for reference.  It may be useful.
// Warning: the NDEBUG flag also turns off assert protection.


void print_freelist(); 

#ifdef NDEBUG
	#define DEBUG(M, ...)
	#define PRINT_FREELIST print_freelist
#else
	#define DEBUG(M, ...) fprintf(stderr, "[DEBUG] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
	#define PRINT_FREELIST
#endif


void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",
	  freelist_head->size,
	  freelist_head,
	  freelist_head->prev,
	  freelist_head->next);
    freelist_head = freelist_head->next;
  }
  DEBUG("\n");
}
