#include <unistd.h>
#include <cstring>//for memcpy and memset
#include <sys/mman.h>

class Bin;
class Block;
typedef unsigned int index_t;

static const size_t MAX_BLOCK_SIZE = 100000000;
static const unsigned int NUM_BINS = 129;
static const index_t BIG_BOI_BIN_INDEX = NUM_BINS-1;
static const index_t NO_LEGAL_INDEX = NUM_BINS;

static Block* last_sbrk_block = nullptr;

static Bin* getBinTable();
size_t _size_meta_data();

class Block{
public:
    static const unsigned int min_udata_size_for_split = 128;

    bool is_free;
    Block* next;
    //the next block should be bigger or equeals in size (next in the bin this block is in).
    
    Block* prev;
    //the previous block should be smaller or equals in size (prev in the bin this block is in).
    
    size_t udata_size;
    Bin* containing_bin;
    Block* before_in_memory;
    //the block with the largest adress that is smaller than 'this', null if one does not exit.

    Block(bool is_free, Block* next, Block* prev, size_t udata_size, Bin* containing_bin, Block* before_in_memory)
        :is_free(is_free), next(next), prev(prev), udata_size(udata_size), 
            containing_bin(containing_bin), before_in_memory(before_in_memory){}

    /**
     * returns the total size of the block, including both the meta data and udata (user-data).
     */
    size_t getTotalSize() const{
        return sizeof(Block) + udata_size;
    }
    void* getStartOfUdata() const{
        return (void*)(((size_t)this) + sizeof(Block));
    }
    /**
     * if the total size is sufficiant to house both a block of size 'first_block_total_size',
     *  and another block with udata size of atleast 'min_udata_size_for_split'
     *       - then the method will make the split: fixing the size of 'this' to be the parameter,
     *       initialzing the new block, making sure that both blocks are in the correct position in the correct bin,
     *       and return a pointer to the new block.
     *  if a split cannot be made - does nothing to the block and returns null.
     */
    Block* splitAndCorrectIfPossible(size_t first_block_total_size){
        //if the split can be made:
        if(getTotalSize() >= first_block_total_size + sizeof(Block) + min_udata_size_for_split){
            //find the position of the new block and initialize it's value:
            Block* new_block = (Block*)((size_t)this + first_block_total_size);
            //fix 'before_in_memory' field of the block after 'this' (if there is a block after):
            if(getAfterInMemory() != nullptr)
                getAfterInMemory()->before_in_memory = new_block;
            size_t new_block_udata_size = this->getTotalSize()-sizeof(Block)-first_block_total_size;//change here!
            *new_block = Block(true, nullptr, nullptr, new_block_udata_size, nullptr, this);
            //update the size of the existing block:
            this->udata_size = first_block_total_size - sizeof(Block); //change here!
            
            //make sure all of the blocks are in the correct positions:
            this->correctPositionInBinTable();
            new_block->correctPositionInBinTable();
            return new_block;
        } else
            return nullptr;
    }

    /**
     * returns the block that is right after 'this' in memory,
     * if one does not exist or if 'this' is the the mmap'ed bin, returns null.
     */
    Block* getAfterInMemory() const;

    /**
     * if this is free and block after it in memory exits and is free - merege with it, correcting all fields.
     */
    void tryMeregeWithAfterInMemory(){
        if(!is_free)
            return;
        Block* after = getAfterInMemory();    
        if(after == nullptr || !after->is_free)
            return;
         
        //otherwise, we have to merege them together:
        after->removeFromContainingBin();
        
        //if there is a block after 'this->after', fix it's 'before_in_memory' field:
        if(after->getAfterInMemory() != nullptr)
            after->getAfterInMemory()->before_in_memory = this;

        //this block 'takes on' the size of the block after it:
        udata_size += after->getTotalSize();
        //the size of 'this' has changed, so we need to correct it's position to the right place:
        correctPositionInBinTable();
    }
    
    /**
     * if this is not free, do nothing.
     * otherwise, for any of it's neighbor that are free (next or prex), merege them together
     *  (and correct all pointers, size values etc).
     */
    void tryMeregeWithNeighbors(){
        tryMeregeWithAfterInMemory();
        if(before_in_memory != nullptr)
            before_in_memory->tryMeregeWithAfterInMemory();
    }

    /**
     * returns wether the bin should be in the bin it is currently in, according to it's size.
     * returns false if containing_bin is null. (it is in no bin, so definitely not the correct one)
     */
    bool isInCorrectBin() const;

    /**
     * swaps 'this' with the next block, correcting for pointers of both blocks,
     *  the blocks around them and the pointers in the bin that contains them.
     */
    void siftWithNext();
    
    /**
     * swaps 'this' with the prev block, correcting for pointers of both blocks,
     *  the blocks around them and the pointers in the bin that contains them.
     */
    void siftWithPrev();

    /**
     * changes the position of this Block within it's containing bin, untils the position satisfies:
     *      1. if there is a next block, this block is smaller than the next.
     *      2. if there is a previous block, this block is bigger than the prev.
     * if this block is already in the correct position - does nothing.
     * if this block is not in the correct bin, the behavior is undefiend.
     */
    void siftToCorrectPositionWithinBin(){
        while(true){
            if(next != nullptr && next->getTotalSize() < getTotalSize())
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
    void removeFromContainingBin();

    /**
     * assumes the parameter block has the correct value for the field 'udata_size'.
     * puts the block in the appropriate bin for it's actual size, and in the correct position within that bin.
     * will work if the block is in no bin or position {next, prev, containing_bin} are null.
     * does nothing if the block is already in the correct place.
     */
    void correctPositionInBinTable();

    /**
     * returns a pointer to the block s.t. 'udata_ptr' points to the udata of that block.
     * if the parameter is null, returns null.
     */
    static Block* getBlockFromAllocatedUdata(void* udata_ptr){
        if(udata_ptr == nullptr)
            return nullptr;
        return (Block*)((size_t)udata_ptr-sizeof(Block));
    }
};

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
        if(bin_index == BIG_BOI_BIN_INDEX)
            return MAX_BLOCK_SIZE;
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

private:
    /**
     * finds the smallest block in this bin (only looks in this bin!) that a block of 'total_size' can fit in, and returns a pointer to it.
     * if one does not exits, returns null;
     */
    Block* findSmallestBlockThatFitsInBin(size_t total_size){
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
public:
    /**
     * looks for a block that can fit an object with total size that is atleast the parameter 'total_size'.
     * starts looking within this bin, and if one is not found it continues looking in the next bin,
     * unless the next bin is the one with index, BIN_BOI_BIN_INDEX, in which case the method returns null.
     */
    Block* findSmallestFitInTable(size_t total_size){
        //there is not fit in BIG_BOI_BIN, and all of blocks should be non_free anyways.
        if(bin_index == BIG_BOI_BIN_INDEX)
            return nullptr;
        //try finding the best fit in this bin:
        Block* res = findSmallestBlockThatFitsInBin(total_size);
        if(res != nullptr)
            return res;
        //try looking in the next block:
        return getBinTable()[bin_index+1].findSmallestFitInTable(total_size);
    }
    
    /**
     * returns the index of the bin that should hold a block with 
     *      TOTAL size that equals to the 'size' parameter.
     * the 'index' of the bin should be s.t. 'total_size' is in '[KB_bytes*index, KB_bytes*(index+1))'. 
     */
    static index_t getBinIndexForSize(size_t total_size){
        if(total_size > MAX_BLOCK_SIZE)
            return NO_LEGAL_INDEX;
        size_t bin_index = total_size/KB_bytes;
        if(bin_index > BIG_BOI_BIN_INDEX)
            bin_index = BIG_BOI_BIN_INDEX;
        return bin_index;  
    }

    /**
     * tries to allocate new space for a block with TOTAL 'total_size' by calling 'sbrk()' or 'mmap()',
     *      if they fail or if size is more than MAX_BLOCK_SIZE - returns null.
     * otherwise - initializes a new block in the allocated space, and returns a pointer to it.
     * DOES NOT insert the block to it's correct place in the bin-table.
     */
    Block* allocateNewBlock(size_t total_size){
        Block* new_block;
        //in the the case that the new block is too big for the regular bins:
        index_t bin_index = getBinIndexForSize(total_size);

        size_t size_missing = 0;
        Block* before_in_mem = nullptr;
        switch (bin_index)
        {
        case NO_LEGAL_INDEX://the block is too big:
            return nullptr;
            break;
        case BIG_BOI_BIN_INDEX://the block should be allocated with mmap:
            new_block = (Block*)mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if(new_block == nullptr)
                return nullptr;
            break;
        default://the block should be in a 'regular' bin:
            //find the address where the new block will start, and how much more space it will need:
            if(last_sbrk_block != nullptr && last_sbrk_block->is_free){
                last_sbrk_block->removeFromContainingBin();
                size_missing = (size_t)last_sbrk_block + total_size - (size_t)sbrk(0);
                new_block = last_sbrk_block;
            } else {
                size_missing = total_size;
                new_block = (Block*)sbrk(0);
            } 
            //allocate that new space as needed:
            if(sbrk(size_missing) == nullptr)
                return nullptr;
            before_in_mem = last_sbrk_block;
            last_sbrk_block = new_block;
        }
        //initialize the values of the new block:
        *new_block = Block(false, nullptr, nullptr, total_size-sizeof(Block), nullptr, before_in_mem);
        return new_block;
    }
};

Block* Block::getAfterInMemory() const{
    if(containing_bin != nullptr && containing_bin->bin_index == BIG_BOI_BIN_INDEX)
        return nullptr;
    Block* possible_new_block = (Block*)((size_t)this+getTotalSize());
    if(possible_new_block == sbrk(0))
        return nullptr;
    return possible_new_block;
}

bool Block::isInCorrectBin() const{
    if(containing_bin == nullptr)
        return false;
    
    return this->containing_bin->inBinSizeRange(this->getTotalSize()); 
}

void Block::siftWithNext(){
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

void Block::siftWithPrev(){
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
    prev = new_prev;
    next = old_prev;
    old_prev->prev = this;
    old_prev->next = old_next;
}

void Block::removeFromContainingBin(){
    Bin* bin = this->containing_bin;
    if(bin == nullptr)
        return;//a block with no bin should have no blocks connected to it.
    
    if(bin->biggest == this)
        bin->biggest = this->prev;
    if(bin->smallest == this)
        bin->smallest = this->next;
    this->containing_bin = nullptr;

    if(this->next != nullptr)
        this->next->prev = this->prev;
    if(this->prev != nullptr)
        this->prev->next = this->next;

    this->next = nullptr;
    this->prev = nullptr;
}

void Block::correctPositionInBinTable(){
    if(!isInCorrectBin()){
        removeFromContainingBin();
        
        containing_bin = Bin::getProperBinForSize(this->getTotalSize());
        
        //insert the block to the bin as the 'new smallest'
        // - with possibly incorrect position because it might now be the smallest:
        if(containing_bin->smallest != nullptr){
            Block* old_smallest = containing_bin->smallest;
            old_smallest->prev = this;
            this->next = old_smallest;
        } else 
            containing_bin->biggest = this;
        containing_bin->smallest = this;
    }
    
    siftToCorrectPositionWithinBin();
    //this part should work whether or not the block was inserted to a new bin.
}

/**
 * the allocation system has a single bin table that is infact an array of bins,
 *      this data structure allows access to every block allocated by the system.
 *      this data structure is a singleton, and this function is the only way to access this singleton.
 * the function will initialize the bin table the first time it is called, and from there on simple return it.
 */
static Bin* getBinTable(){
    //bin 128=BIG_BOI_BIN_INDEX (the last one) is special, it hold all of the blocks that are too bin to be in any of the other bins.
    static Bin bin_table[NUM_BINS];
    //this line should call the default constructor 'NUM_BINS' times, each time creating a bin with the next index.    

    return bin_table;
}

/**
 * @brief given the wanted size of the user data, first tries to find a existing block that can fit it,
 * and allocates a new one if needed. in addition does the following:
 *      1. this function will do all of the corrections needed to maintain the data structure.
 *      2. if an existing block with enough size was found, the function will split it as needed.
 *      3. the appropriate allocation will be done with either sbrk or mmap as needed.
 *      4. if a new (sbrk) allocation is made and the block closest to brk is free, the new block will merege with the last.
 * @return the functions will normally return a pointer to the user-data that was requested, but will fail
 *  (and return null) in the following cases:
 *      1. udata_size is more than MAX_BLOCK_SIZE.
 *      2. srbk() or mmap() have failed.
 */
static void* completeSearchAndAllocate(size_t udata_size){
    if(udata_size == 0 || udata_size > MAX_BLOCK_SIZE)
        return nullptr;
    size_t min_block_total_size = udata_size+sizeof(Block);
    Bin* bin = Bin::getProperBinForSize(min_block_total_size);
    
    Block* res_block;
    //if we dont have to allocate a new block:
    if(bin->bin_index != BIG_BOI_BIN_INDEX
    &&(res_block = bin->findSmallestFitInTable(min_block_total_size)) != nullptr){
        res_block->splitAndCorrectIfPossible(min_block_total_size);  
        res_block->is_free=false;
    }
    //if we have to allocate a new block:
    else {
        res_block = bin->allocateNewBlock(min_block_total_size);
        res_block->correctPositionInBinTable();
    }

    return res_block->getStartOfUdata();
}

/**
 * changes the values of all bytes in memory to 0: starting with 'start_addr', 
 *      and ending with 'start_addr+num_bytes-1' (including the last one).
 */
static void zeroOutBytes(void* start_addr, size_t num_bytes){
    memset(start_addr, 0, num_bytes);
}

/**
 * copies 'num_bytes' bytes from 'src' to 'dst'.
 */
static void copyBytes(void* src, void* dst, size_t num_bytes){
    memcpy(dst, src, num_bytes);
}

#define forEachBlock(todo) \
    for(index_t __bindex = 0; __bindex < NUM_BINS; ++__bindex){\
        Block* cur_block = getBinTable()[__bindex].smallest;\
        while(cur_block != nullptr){\
            todo;\
            cur_block = cur_block->next;\
        }\
    }\

/*
============ Interface: ==================================================================================================================================
*/

/**
 * @brief Searches for a free block up to 'size' bytes or allocates a new one if serach fails.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns null in the following cases: 'size' is 0 or more than MAX_BLOCK_SIZE, 'sbrk()' fails.
 */
void* smalloc(size_t size){
    return completeSearchAndAllocate(size);
}

/**
 * @brief Searches for a free block of up to 'num' elements, each 'size' bytes that are all set to 0,
 *      or allocates if none are found. in other words, find/allocate size*num bytes and set all bytes to 0.
 * @return
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns null in the following cases: 'size' is 0 or more than MAX_BLOCK_SIZE, 'sbrk()' fails.
 */
void* scalloc(size_t num, size_t size){
    void* udata_res = completeSearchAndAllocate(size*num);
    if(udata_res != nullptr){//if a space was actualy found, zero it out:
        Block* block_res = Block::getBlockFromAllocatedUdata(udata_res);
        if(block_res->containing_bin->bin_index != BIG_BOI_BIN_INDEX){
            //(in the case of an mmap allocation, the space is already zeroed so we can skip this)
            zeroOutBytes(block_res->getStartOfUdata(), block_res->udata_size);
        } 
    }
    return udata_res;
}

/**
 * @brief Releases the usage of the block that starts with the pointer 'p'.
 *      if 'p' is null or already released, do nothing.
 * assumes 'p' truely points to the start or an allocated block.
 */
void sfree(void* p){
    if(p == nullptr)
        return;
    Block* block = Block::getBlockFromAllocatedUdata(p);
    if(block->is_free)
        return;
    block->is_free = true;

    if(block->containing_bin->bin_index == BIG_BOI_BIN_INDEX){
        block->removeFromContainingBin();
        munmap(block, block->getTotalSize());
    } else {
        block->tryMeregeWithNeighbors();
    }
}

/**
 * @brief f ‘size’ is smaller than the current block’s size, reuses the same block. Otherwise,
 *      finds/allocates ‘size’ bytes for a new space, copies content of oldp into the new
 *      allocated space and frees the oldp.
 * 'oldp' is not freed if srealloc fails.
 * if 'oldp' is null, allocates space for 'size' bytes and return pointer. 
 * @return 
 *      success - returns a pointer to the first byte in the allocated block (excluding meta-data).
 *      failure - returns null in the following cases: 'size' is 0 or more than MAX_BLOCK_SIZE, 'sbrk()' fails.
 * does not free 'oldp' is srealloc fails.
 */
void* srealloc(void* oldp, size_t size){
    if(oldp == nullptr || size >= MAX_BLOCK_SIZE || size == 0)
        return nullptr;
    
    size_t wanted_total_size = size + sizeof(Block);
    Block* cur_block = Block::getBlockFromAllocatedUdata(oldp);
    size_t old_udata_size = cur_block->udata_size;
    Block* new_block = nullptr;

    auto min = [](int x, int y){return x < y ? x : y;};

    //if the current block is from the mmap region:
    if(cur_block->containing_bin == nullptr || cur_block->containing_bin->bin_index == BIG_BOI_BIN_INDEX){
        void* res = completeSearchAndAllocate(size);
        if(res == nullptr)
            return nullptr;
        memmove(res, oldp, min(size, old_udata_size));
        sfree(oldp);
        return res;
    }

    //if there is no need to change the block:
    if(cur_block->getTotalSize() >= wanted_total_size){
        cur_block->splitAndCorrectIfPossible(wanted_total_size);
        return oldp;
    }
    cur_block->is_free = true;//for now, until memory will be copied.
    //if we should merege with the block before 'this' in memory:
    if(cur_block->before_in_memory != nullptr
    && cur_block->before_in_memory->is_free
    && cur_block->before_in_memory->getTotalSize() + cur_block->getTotalSize() >= wanted_total_size){
        new_block = cur_block->before_in_memory;
        new_block->tryMeregeWithAfterInMemory();
        cur_block->is_free = false; // restore actuall is_free, might be redundent because already merged
    } else
    //if we should merege with the block after 'this' in memory: 
    if(cur_block->getAfterInMemory() != nullptr
    && cur_block->getAfterInMemory()->is_free
    && cur_block->getAfterInMemory()->getTotalSize() + cur_block->getTotalSize() >= wanted_total_size){
        new_block = cur_block;
        new_block->tryMeregeWithAfterInMemory();
    } else
    //if we should merege with both the neighbors of 'this':
    if(cur_block->getAfterInMemory() != nullptr
    && cur_block->before_in_memory != nullptr
    && cur_block->getAfterInMemory()->is_free
    && cur_block->before_in_memory->is_free
    && cur_block->getAfterInMemory()->getTotalSize() + cur_block->getAfterInMemory()->getTotalSize()
        + cur_block->getTotalSize() >= wanted_total_size){ 
        new_block = cur_block->before_in_memory;
        new_block->tryMeregeWithNeighbors();
    } else {//we have to look for space in the regular way (first in bin table and sbrk/mmap otherwise):
        new_block = Block::getBlockFromAllocatedUdata(completeSearchAndAllocate(size));
        if(new_block == nullptr){
            new_block->is_free = false;
            return nullptr;
        }
        sfree(oldp);
    }
    new_block->is_free = false;        

    //at this point, we have found some block with the reqested size (new_block), the old block was removed if it was needed.
    //now we update the allocation data structure to handle the change, and copy the old user data:
    new_block->splitAndCorrectIfPossible(wanted_total_size);
    memmove(new_block->getStartOfUdata(), oldp, min(size, old_udata_size));
    
    return new_block->getStartOfUdata();
}   


/**
 * Returns the number of allocated blocks in the heap that are currently free.
 */
size_t _num_free_blocks(){
    unsigned int counter = 0;
    forEachBlock(counter += cur_block->is_free);
    return counter;
}
/**
 * Returns the number of bytes in all allocated blocks in the heap that are currently free,
 * excluding the bytes used by the meta-data structs.
 */
size_t _num_free_bytes(){
    size_t tot_free = 0;
    forEachBlock(tot_free += (cur_block->is_free ? cur_block->udata_size : 0));
    return tot_free;
}
/**
 * Returns the overall (free and used) number of allocated blocks in the heap.
 */
size_t _num_allocated_blocks(){
    unsigned int counter = 0;
    forEachBlock(++counter);
    return counter;
}
/**
 * Returns the overall number (free and used) of allocated bytes in the heap, excluding
 * the bytes used by the meta-data structs.
 */
size_t _num_allocated_bytes(){
    size_t tot_alloc = 0;
    forEachBlock(tot_alloc += cur_block->udata_size);
    return tot_alloc;
}
/**
 * Returns the number of bytes of a single meta-data structure in your system.
 */
size_t _size_meta_data(){
    return sizeof(Block);
}
/**
 * Returns the overall number of meta-data bytes currently in the heap.
 */
size_t _num_meta_data_bytes(){
    return _size_meta_data()*_num_allocated_blocks();
}

