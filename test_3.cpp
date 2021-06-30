#include <iostream>
#include "malloc_3.cpp"
#include "assert.h"
#include <execinfo.h>

using namespace std;

int getDepth(){
    int max_depth = 200;
    void* buffer[max_depth];
    int ret = backtrace(buffer, max_depth);
    return ret-5;
}

void printIndentation(){
    for(int i = 0; i < getDepth(); ++i)
    cout << "   ";
}

#define outi(line) printIndentation(); cout << line;

void printBlock(Block* block){
    outi("printBlock: " << "status: " << (block->is_free ? "free" : "used")
        << ". udata-size: " << block->udata_size << ". block-address: 0x");
    printf("%lx.\n", (size_t)block);
}

void printBin(Bin* bin){
    outi("printBin: bin-index: " << bin->bin_index << endl);
    Block* block = bin->smallest;
    while(block != nullptr){
        printBlock(block);
        block = block->next;
    }
}

void printTo(index_t index = 4){
    outi("printTo:" << endl;)
    for(Bin* bin = getBinTable(); bin != getBinTable()+index; ++bin){
        printBin(bin);
    }
    printBin(getBinTable()+BIG_BOI_BIN_INDEX);
}

void check1(){
    outi("check1:" << endl);
    for(Bin* bin = getBinTable(); bin != getBinTable()+NUM_BINS; ++bin){
        if(bin->biggest != nullptr){
            assert(bin->biggest->next == nullptr);
            assert(bin->smallest->prev == nullptr);        
        } else {
            assert(bin->smallest == nullptr);
        }
    }    
}


int main(){
    outi("main:" << endl);
    printTo();
    void* p1 = smalloc(1);
    printTo();
    void* p2 = smalloc(42);
    Block* loc = Block::getBlockFromAllocatedUdata(p2);
    printTo();
    void* p3 = smalloc(Bin::KB_bytes*1);
    printTo();
    sfree(p1);
    printTo();
    sfree(p2);
    printTo();

    cout << "finished successfully\n";
}