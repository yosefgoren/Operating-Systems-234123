#include <unistd.h>



/*
============ Interface: ==================================================================================================================================
*/

/**
 * @brief Searches for a free block up to 'size' bytes or allocates a new one if serach fails.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns NULL in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 */
void* smalloc(size_t size){
    
}

/**
 * @brief Searches for a free block of up to 'num' elements, each 'size' bytes that are all set to 0,
 *      or allocates if none are found. in other words, find/allocate size*num bytes and set all bytes to 0.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns NULL in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 */
void* scalloc(size_t num, size_t size){

}

/**
 * @brief Releases the usage of the block that starts with the pointer 'p'.
 *      if 'p' is NULL or already released, do nothing.
 * assumes 'p' truely points to the start or an allocated block.
 */
void sfree(void* p){

}

/**
 * @brief f ‘size’ is smaller than the current block’s size, reuses the same block. Otherwise,
 *      finds/allocates ‘size’ bytes for a new space, copies content of oldp into the new
 *      allocated space and frees the oldp.
 * 'oldp' is not freed if srealloc fails.
 * if 'oldp' is NULL, allocates space for 'size' bytes and return pointer. 
 * @return 
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns NULL in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 */
void* srealloc(void* oldp, size_t size){

}


/**
 * Returns the number of allocated blocks in the heap that are currently free.
 */
size_t _num_free_blocks(){

}
/**
 * Returns the number of bytes in all allocated blocks in the heap that are currently free,
 */

size_t _num_free_bytes(){

}

/**
 * Returns the overall (free and used) number of allocated blocks in the heap.
 * excluding the bytes used by the meta-data structs.
 */
size_t _num_allocated_blocks(){

}
/**
 * Returns the overall number (free and used) of allocated bytes in the heap, excluding
 * the bytes used by the meta-data structs.
 */
size_t _num_allocated_bytes(){

}
/**
 * Returns the overall number of meta-data bytes currently in the heap.
 */
size_t _num_meta_data_bytes(){

}
/**
 * Returns the number of bytes of a single meta-data structure in your system.
 */
size_t _size_meta_data(){

}


