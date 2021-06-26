#include <unistd.h>
#include <cstring>

using namespace std;

struct Block{
    static const size_t meta_data_size = sizeof(bool)+sizeof(Block*)+sizeof(Block*);
    
    bool is_free;
    Block* next;
    Block* prev;

    size_t getFullSize() const{
        void* first_byte_outside_udata = (next == NULL
            ? sbrk(0) : (void*)next);
        return (size_t)first_byte_outside_udata - (size_t)this;
    }
    size_t getUdataSize() const{
        return getFullSize() - meta_data_size;
    }
    void* getStartOfUdata() const{
        return (void*)this + meta_data_size;
    }
    void setFreeAndRemoveFromList(){
        is_free = true;
        if(next != NULL)
            next->prev = prev;
        if(prev != NULL)
            prev->next = next;
        next = prev = NULL;
    }

    /**
     * returns a pointer to the block s.t. 'udata_ptr' points to the udata of that block.
     */
    static Block* getBlockFromAllocatedUdata(void* udata_ptr){
        return (Block*)((size_t)udata_ptr-meta_data_size);
    }
};

struct BlockList{
    Block* first;
    Block* last;
};

static BlockList block_list = {NULL, NULL};

/**
 * iterates over the list and returns a pointer to a block with udata with size of atleast 'size'.
 * if one cannot be found, returns NULL.
 */
static Block* findBlockOfSize(size_t size){
    Block* cur_block = block_list.first;
    while(cur_block != NULL){
        if(cur_block->is_free && cur_block->getUdataSize() >= size)
            return cur_block;
    } 
    return NULL;
}

/**
 * tries to allocate new space for a block with udata of size 'size' by calling 'sbrk()',
 *      if fails, reuturns NULL, otherwise - initializes a new block in the allocated space,
 *      inserts it as the last item in the block list, and returns a pointer to it.
 */
static Block* allocateNewBlock(size_t size){
    Block* new_block = (Block*)sbrk(Block::meta_data_size+size);
    if(new_block == NULL)
        return NULL;

    new_block->is_free = false;
    new_block->next = NULL;
    new_block->prev = block_list.last;
    
    if(block_list.last != NULL)
        block_list.last->next = new_block;
    else
        block_list.first = block_list.last = new_block;

    return new_block;
}

/**
 * changes the values of all bytes in memory to 0: starting with 'start_addr', 
 *      and ending with 'start_addr+num_bytes-1' (including the last one).
 */
static void zeroOutBytes(void* start_addr, size_t num_bytes){
    // for(size_t i = 0; i < num_bytes; ++i)
    //     *((char*)start_addr+i) = 0;
    memset(start_addr, 0, num_bytes);
}

/**
 * copies 'num_bytes' bytes from 'src' to 'dst'.
 */
static void copyBytes(void* src, void* dst, size_t num_bytes){
    memcpy(dst, src, num_bytes);
    // for(size_t i = 0; i < num_bytes; ++i)
    //     *((char*)dst+i) = *((char*)src+i);
}

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
    if(size == NULL || size > 100000000)
        return NULL;
    
    Block* res = findBlockOfSize(size);
    if(res != NULL){
        res->is_free = false;
        return res->getStartOfUdata();
    }
    res = allocateNewBlock(size);
    if(res == NULL)
        return NULL;

    return res->getStartOfUdata();
}

/**
 * @brief Searches for a free block of up to 'num' elements, each 'size' bytes that are all set to 0,
 *      or allocates if none are found. in other words, find/allocate size*num bytes and set all bytes to 0.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns NULL in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 */
void* scalloc(size_t num, size_t size){
    if(size == NULL || size > 100000000)
        return NULL;   
    Block* res = findBlockOfSize(num*size);
    if(res != NULL){
        zeroOutBytes(res->getStartOfUdata(), num*size);
        res->is_free = false;
        return res->getStartOfUdata();
    }

    res = allocateNewBlock(num*size);
    if(res == NULL)
        return NULL;
    
    zeroOutBytes(res->getStartOfUdata(), num*size);
    return res->getStartOfUdata();
}

/**
 * @brief Releases the usage of the block that starts with the pointer 'p'.
 *      if 'p' is NULL or already released, do nothing.
 * assumes 'p' truely points to the start or an allocated block.
 */
void sfree(void* p){
    if(p == NULL)
        return;
    Block::getBlockFromAllocatedUdata(p)->is_free = true;
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
 * does not free 'oldp' is srealloc fails.
 */
void* srealloc(void* oldp, size_t size){
    if(size == 0 || size > 100000000)
        return NULL;
    if(oldp == NULL)
        return smalloc;

    Block* old_block = Block::getBlockFromAllocatedUdata(oldp);
    size_t old_size = old_block->getUdataSize();
    if(old_size >= size)
        return oldp;

    Block* new_block = findBlockOfSize(size);
    if(new_block == NULL){
        new_block = allocateNewBlock(size);
        if(new_block == NULL)
            return NULL;
    }

    copyBytes(oldp, new_block->getStartOfUdata(), old_size);
    sfree(oldp);
    return new_block->getStartOfUdata();
}   


/**
 * Returns the number of allocated blocks in the heap that are currently free.
 */
size_t _num_free_blocks(){
    int counter = 0;
    Block* cur_block = block_list.first;
    while(cur_block != NULL){
        counter += cur_block->is_free;
        cur_block = cur_block->next;
    }
    return counter;
}
/**
 * Returns the number of bytes in all allocated blocks in the heap that are currently free,
 * excluding the bytes used by the meta-data structs.
 */
size_t _num_free_bytes(){
    size_t size_free = 0;
    Block* cur_block = block_list.first;
    while(cur_block != NULL){
        if(cur_block->is_free)
            size_free += cur_block->getUdataSize();
        cur_block = cur_block->next;
    }
    return size_free;
}
/**
 * Returns the overall (free and used) number of allocated blocks in the heap.
 */
size_t _num_allocated_blocks(){
    int counter = 0;
    Block* cur_block = block_list.first;
    while(cur_block != NULL){
        counter += 1;
        cur_block = cur_block->next;
    }
    return counter;
}
/**
 * Returns the overall number (free and used) of allocated bytes in the heap, excluding
 * the bytes used by the meta-data structs.
 */
size_t _num_allocated_bytes(){
    size_t size_free = 0;
    Block* cur_block = block_list.first;
    while(cur_block != NULL){
        size_free += cur_block->getUdataSize();
        cur_block = cur_block->next;
    }
    return size_free;
}
/**
 * Returns the overall number of meta-data bytes currently in the heap.
 */
size_t _num_meta_data_bytes(){
    return _num_allocated_blocks()*Block::meta_data_size;
}
/**
 * Returns the number of bytes of a single meta-data structure in your system.
 */
size_t _size_meta_data(){
    return Block::meta_data_size;
}


