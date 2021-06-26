#include <unistd.h>

struct Bin;
struct Block;
/**
 * assumes the parameter block has the correct value for the field 'udata_size'.
 * puts the block in the appropriate bin for it's actual size, and in the correct position within that bin.
 * will work if the block is in no bin or position {next, prev, containing_bin} are null.
 * does nothing if the block is already in the correct place.
 */
static void correctPositionInBinTable(Block* block){
    //TODO:
}

struct Block{
    static const unsigned int min_udata_size_for_split = 128;

    bool is_free;
    Block* next;
    Block* prev;
    size_t udata_size;
    Bin* containing_bin;

    Block(bool is_free, Block* next, Block* prev, size_t udata_size, Bin* containing_bin)
        :is_free(is_free), next(next), prev(prev), udata_size(udata_size), containing_bin(containing_bin){}

    /**
     * returns the total size of the block, including both the meta data and udata (user-data).
     */
    size_t getTotalSize() const{
        return sizeof(Block) + udata_size;
    }
    void* getStartOfUdata() const{
        return (void*)this + sizeof(Block);
    }
    /**
     * if the total size is sufficiant to house both a block of size 'first_block_total_size',
     *  and another block with udata size of atleast 'min_udata_size_for_split'
     *       - then the method will make the split: fixing the size of 'this' to be the parameter,
     *       initialzing the new block, make sure both blocks are in the correct position in the correct bin,
     *       and return a pointer to the new block.
     *  if a split cannot be made - does nothing to the block and returns null.
     */
    Block* splitAndCorrectIfPossible(size_t first_block_total_size){
        //if the split can be made:
        if(getTotalSize() >= first_block_total_size + sizeof(Block) + min_udata_size_for_split){
            //find the position of the new block and initialize it's value:
            Block* new_block = (Block*)((void*)this + sizeof(Block) + first_block_total_size);
            size_t new_block_udata_size = this->getTotalSize()-sizeof(Block)*2-first_block_total_size;
            *new_block = Block(true, nullptr, nullptr, new_block_udata_size, nullptr);
            //update the size of the existing block:
            this->udata_size = first_block_total_size; 
            
            //make sure all of the blocks are in the correct positions:
            correctPositionInBinTable(this);
            correctPositionInBinTable(new_block);
        } else
            return nullptr;
    }

    /**
     * returns a pointer to the block s.t. 'udata_ptr' points to the udata of that block.
     */
    static Block* getBlockFromAllocatedUdata(void* udata_ptr){
        return (Block*)((size_t)udata_ptr-sizeof(Block));
    }
};

typedef unsigned int index_t;

/**
 * a double-linked-list of Blocks. each block must maintain that
 *      the total size of the block is within: '[KB_bytes*bin_index, KB_bytes*(bin_index+1))'.
 */
struct Bin{
    static const unsigned int KB_bytes = 1024;

    const index_t bin_index;
    Block* biggest;
    Block* smallest;
    Bin(index_t bin_index):bin_index(bin_index), biggest(nullptr), smallest(nullptr){}
    
    /**
     * returns the index of the bin that should hold a block with 
     *      TOTAL size that equals to the 'size' parameter.
     * the 'index' of the bin should be s.t. 'total_size' is in '[KB_bytes*index, KB_bytes*(index+1))'. 
     */
    static index_t getBinIndexForSize(size_t total_size){
        if(total_size/KB_bytes != (total_size-1)/KB_bytes)
            return total_size/KB_bytes+1;
        return total_size/KB_bytes;
    }

    /**
     * returns the maximal total block size that should be in this bin.
     */
    size_t binMaxSize() const{
        return KB_bytes*(bin_index+1)-1;
    }
    /**
     * returns the minimal total block size that should be in this bin.
     */
    size_t binMinSize() const{
        return KB_bytes*bin_index;
    }
    /**
     * returns true if a block with total size 'total_size' should be in this bin.
     * returns false otherwise.
     */
    bool inBinSizeRange(size_t total_size) const{
        return total_size >= binMaxSize() && binMaxSize() >= total_size;
    }
    /**
     * finds the smallest block in the bin that a block of 'total_size' can fit in, and returns a pointer to it.
     * if one does not exits, returns null;
     */
    Block* findSmallestBlockThatFits(size_t total_size){
        Block* cur_block = smallest;
        while(cur_block != nullptr){
            if(cur_block->is_free && cur_block->getTotalSize() >= total_size){
                cur_block->splitAndCorrectIfPossible(total_size);
                return cur_block;
            }
            cur_block = cur_block->next;
        }
        return nullptr;
    }
};

static const unsigned int num_bins = 128;
static Bin bin_table[num_bins];
static bool bins_initialized = false;

//TODO:
static void initializeBinsIfNeeded(){
    
}

/*
============ Interface: ==================================================================================================================================
*/

/**
 * @brief Searches for a free block up to 'size' bytes or allocates a new one if serach fails.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns null in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 */
void* smalloc(size_t size){

}

/**
 * @brief Searches for a free block of up to 'num' elements, each 'size' bytes that are all set to 0,
 *      or allocates if none are found. in other words, find/allocate size*num bytes and set all bytes to 0.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns null in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 */
void* scalloc(size_t num, size_t size){

}

/**
 * @brief Releases the usage of the block that starts with the pointer 'p'.
 *      if 'p' is null or already released, do nothing.
 * assumes 'p' truely points to the start or an allocated block.
 */
void sfree(void* p){

}

/**
 * @brief f ‘size’ is smaller than the current block’s size, reuses the same block. Otherwise,
 *      finds/allocates ‘size’ bytes for a new space, copies content of oldp into the new
 *      allocated space and frees the oldp.
 * 'oldp' is not freed if srealloc fails.
 * if 'oldp' is null, allocates space for 'size' bytes and return pointer. 
 * @return 
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns null in the following cases: 'size' is 0 or more than 100000000, 'sbrk()' fails.
 * does not free 'oldp' is srealloc fails.
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
 * excluding the bytes used by the meta-data structs.
 */
size_t _num_free_bytes(){
}
/**
 * Returns the overall (free and used) number of allocated blocks in the heap.
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

