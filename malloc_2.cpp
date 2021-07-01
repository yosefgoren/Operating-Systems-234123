#include <unistd.h>
#include <cstring>//for memcpy and memset
#include <sys/mman.h>

static const size_t MAX_BLOCK_SIZE = 100000000;

struct Block {
    size_t udata_size;
    bool is_free;

    size_t getTotalSize() const{
        return sizeof(Block)+udata_size;
    }

    void* getUdataStart() const{
        return (void*)((size_t)this+sizeof(Block));
    }

    Block* getBlockAfter() const{
        Block* possible_new_block = (Block*)((size_t)this+getTotalSize());
        if(possible_new_block == (Block*)sbrk(0))
            return nullptr;
        return possible_new_block;
    }
};

Block* blockFromUdata(void* udata){
    if(udata == nullptr)
        return nullptr;
    return (Block*)((size_t)udata-sizeof(Block));
}

Block* first_block = nullptr;

#define foreach(todo)\
    Block* block_cur = first_block;\
    while (block_cur != nullptr){\
        todo;\
        block_cur = block_cur->getBlockAfter();\
    }\

Block* firstFits(size_t tot_size){    
    // foreach(
    //     if(block_cur->is_free && block_cur->getTotalSize() >= tot_size){
    //         block_cur->is_free = false;
    //         return block_cur;
    //     }
    // )

    Block* block_cur = first_block;
    while (block_cur != nullptr){
        if(block_cur->is_free && block_cur->getTotalSize() >= tot_size){
            block_cur->is_free = false;
            return block_cur;
        }       
        block_cur = block_cur->getBlockAfter();
    }
    return nullptr;
}

Block* newBlock(size_t tot_size){
    Block* res = (Block*)sbrk(tot_size);
    if(res == nullptr)
        return nullptr;
    if(first_block == nullptr)
        first_block = res;
    res->is_free = false;
    res->udata_size = tot_size-sizeof(Block);
    return res;
}

bool illegalUSize(size_t usize){
    return usize == 0 || usize > MAX_BLOCK_SIZE;
}

void* smalloc(size_t size){
    if(illegalUSize(size))
        return nullptr;
    size_t tot_size = size + sizeof(Block);
    Block* res = firstFits(tot_size);
    if(res != nullptr){
        res->is_free = false;
        return res;
    }
    return newBlock(tot_size)->getUdataStart();
}

void* scalloc(size_t num, size_t size){
    size_t usize = num*size;
    if(illegalUSize(usize))
        return nullptr;
    void* res = smalloc(usize);
    if(res != nullptr){
        memset(res, 0, usize);
    }
    return res;
}

void sfree(void* p){
    if(p == nullptr)
        return;
    blockFromUdata(p)->is_free = true;
}

void* srealloc(void* oldp, size_t size){
    if(illegalUSize(size))
        return nullptr;
    if(oldp == nullptr)
        return nullptr;
    Block* block = blockFromUdata(oldp);
    if(block->udata_size >= size)
        return nullptr;
    void* res = smalloc(size);
    if(res == nullptr)
        return nullptr;
    blockFromUdata(res)->is_free = false;
    memcpy(res, oldp, size < block->udata_size ? size : block->udata_size);
    block->is_free = true;
    return res;
}

#define sumup(toadd)\
    size_t sum = 0;\
    foreach(sum += toadd);\
    return sum;

size_t _num_free_blocks(){
    sumup(block_cur->is_free);
}
size_t _num_free_bytes(){
    sumup(block_cur->is_free*block_cur->udata_size);
}
size_t _num_allocated_blocks(){
    sumup(1);
}
size_t _num_allocated_bytes(){
    sumup(block_cur->udata_size);
}
size_t _num_meta_data_bytes(){
    sumup(sizeof(Block));
}
size_t _size_meta_data(){
    return sizeof(Block);
}