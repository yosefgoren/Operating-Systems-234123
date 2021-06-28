#include <unistd.h>
#include <sys/mman.h>

class Bin;
class Block;

static bool isInCorrectBinTablePosition(Block* block);
static void correctPositionInBinTable(Block* block);

class Block{
public:
    static const unsigned int min_udata_size_for_split = 128;

    bool is_free;
    Block* next;
    //the next block should be bigger or equeals in size.
    
    Block* prev;
    //the previous block should be smaller or equals in size.
    
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
            this->correctPositionInBinTable();
            new_block->correctPositionInBinTable();
        } else
            return nullptr;
    }

    /**
     * returns wether the bin should be in the bin it is currently in, according to it's size.
     * returns false if containing_bin is null. (it is in no bin, so definitely not the correct one)
     */
    bool isInCorrectBin() const{
        if(containing_bin == nullptr)
            return false;
        
        return this->containing_bin->inBinSizeRange(this->getTotalSize()); 
    }

    /**
     * completely swaps the positons of the two blocks with eachother in terms of:
     *      1. the blocks around 'this' and 'other'
     *      2. the internal pointers within 'this' and 'other'
     *      3. the bins that contain 'this' and 'other'
     *  if 'other' is null, does nothing.
     */
    // void swapPositionWithBlock(Block* other){
        
    // }

    /**
     * swaps 'this' with the next block, correcting for pointers of both blocks,
     *  the blocks around them and the pointers in the bin that contains them.
     */
    void siftWithNext(){
        //fix pointers of srounding blocks:
        if(next->next != nullptr)
            next->next->prev = this;
        if(prev != nullptr)
            prev->next = next;

        //fix pointers of containing bin:
        if(next == containing_bin->biggest)
            containing_bin->biggest = this;
        if(this == containing_bin->smallest)
            containing_bin->smallest = next;

        //fix the pointers of this and this->next:
        Block* old_next = next;
        Block* new_next = old_next->next;
        Block* old_prev = prev;
        next = new_next;
        prev = old_next;
        old_next->next = this;
        old_next->prev = old_prev;
    }
    
    /**
     * swaps 'this' with the prev block, correcting for pointers of both blocks,
     *  the blocks around them and the pointers in the bin that contains them.
     */
    void siftWithPrev(){
        //fix pointers of srounding blocks:
        if(prev->prev != nullptr)
            prev->prev->next = this;
        if(next != nullptr)
            next->prev = prev;

        //fix pointers of containing bin:
        if(prev == containing_bin->smallest)
            containing_bin->smallest = this;
        if(this == containing_bin->biggest)
            containing_bin->biggest = prev;

        //fix the pointer of this and this->prev:
        Block* old_prev = prev;
        Block* new_prev = old_prev->prev;
        Block* old_next = next;
        prev = old_prev->prev;
        next = old_prev;
        old_prev->prev = this;
        old_prev->next = old_next;
    }

    /**
     * changes the position of this Block within it's containing bin, untils the position satisfies:
     *      1. if there is a next block, this block is smaller than the next.
     *      2. if there is a previous block, this block is bigger than the prev.
     * if this block is already in the correct position - does nothing.
     * if this block is not in the correct bin, the behavior is undefiend.
     */
    void siftToCorrectPositionWithinBin(){
        while(true){
            if(next != nullptr && next->getTotalSize() < getTotalSize()))
                siftWithNext();
            else if(prev != nullptr && prev->getTotalSize() > getTotalSize())
                siftWithPrev();
            else
                break;
        }
    }

    /**
     * changes containing_bin to null, and correct the fields of the blocks and prior bin
     * so that they do not point at 'this' anymore (and maybe point at something else as needed). 
     */
    void removeFromContainingBin(){
        Bin* bin = this->containing_bin;
        if(bin == nullptr)
            return;//a block with no bin thould have no blocks connected to it.
        
        if(bin->biggest == this)
            bin->biggest = this->prev;
        if(bin->smallest == this)
            bin->smallest = this->next;
        this->containing_bin = nullptr;

        if(this->next != nullptr){
            this->next->prev = this->prev;
            this->next = nullptr;
        }
        if(this->prev != nullptr){
            this->prev->next = this->next;
            this->prev = nullptr;
        }
    }

    /**
     * assumes the parameter block has the correct value for the field 'udata_size'.
     * puts the block in the appropriate bin for it's actual size, and in the correct position within that bin.
     * will work if the block is in no bin or position {next, prev, containing_bin} are null.
     * does nothing if the block is already in the correct place.
     */
    void correctPositionInBinTable(){
        if(!isInCorrectBin()){
            removeFromContainingBin();
            
            Bin* correct_bin = Bin::getProperBinForSize(this->getTotalSize());
            //insert the block to the bin as the 'new smallest'
            // - with possibly incorrect position because it might now be the smallest:
            if(correct_bin->smallest != nullptr){
                Block* old_smallest = correct_bin->smallest;
                old_smallest->prev = this;
                this->next = old_smallest;
            } else 
                correct_bin->biggest = this;
            correct_bin->smallest = this;
        }
        
        siftToCorrectPositionWithinBin();
        //this part should work whether or not the block was inserted to a new bin.
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
class Bin{
public:
    static const unsigned int KB_bytes = 1024;

    const index_t bin_index;
    Block* biggest;
    Block* smallest;

    /**
     * remembers the number of bins that where initialized thus far,
     * returns that number and than increments it when called.
     */
    static index_t binsCounter(){
        static index_t counter = 0;
        return counter++;
    }

    Bin():bin_index(binsCounter()), biggest(nullptr), smallest(nullptr){}

    /**
     * returns the bin that a block thould be it given it's total size.
     */
    static Bin* getProperBinForSize(size_t total_size){
        return getBinTable()+getBinIndexForSize(total_size);
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
     * tries to allocate new space for a block with TOTAL 'total_size' by calling 'sbrk()',
     *      if fails, reuturns NULL, otherwise - initializes a new block in the allocated space,
     *      inserts it as the last item in the block list, and returns a pointer to it.
     */
    Block* allocateNewBlock(size_t total_size){
        Block* new_block; 
        //in the the case that the new block is too big for the regular bins:
        //new_block = (Block*)mmap(nullptr, total_size, PROT_READ | PROT_WRITE, -1, 0);
    }  
};

/**
 * the allocation system has a single bin table that is infact an array of bins,
 *      this data structure allows access to every block allocated by the system.
 *      this data structure is a singleton, and this function is the only way to access this singleton.
 * the function will initialize the bin table the first time it is called, and from there on simple return it.
 */
static Bin* getBinTable(){
    static const unsigned int num_bins = 128;
    //this part should call the default constructor num_bins times.
    static Bin bin_table[num_bins];
    
    return bin_table;
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
    if(size == 0 || size > 100000000)
        return nullptr;
    size_t min_block_total_size = size+sizeof(Block);
    Bin* bin = Bin::getProperBinForSize(min_block_total_size);
    
    Block* housing_block = bin->findSmallestBlockThatFits(min_block_total_size);
    if(housing_block != nullptr){
        housing_block->splitAndCorrectIfPossible(min_block_total_size);
        return housing_block->getStartOfUdata();
    }
    
    housing_block = bin->allocateNewBlock(min_block_total_size);
    if(housing_block == nullptr)
        return nullptr;
    return housing_block->getStartOfUdata();
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

