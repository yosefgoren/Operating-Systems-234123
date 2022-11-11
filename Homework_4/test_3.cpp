#include <iostream>
#include "malloc_3.cpp"
#include "assert.h"
#include <execinfo.h>

using namespace std;

void* heap_start;

void** aref = nullptr;

void printMemory(void *start=heap_start, bool onlyList=true) {
	Block* current = (Block*) start;
	size_t size = 0;
	int blocks = 0;
	if (!onlyList) {
		std::cout << "Printing Memory List\n";
	}
    Block* program_break = (Block*)sbrk(0);
    while (current != program_break) {
        if (current->is_free) {
			std::cout << "|F:" << current->udata_size;
		} else {
			std::cout << "|U:" << current->udata_size;
		}
		size += current->udata_size;
		blocks++;
//		current = current->next;
        size_t effective_size = current->udata_size;
        char * temp_ptr = (char *)current;
        temp_ptr += effective_size + sizeof (Block);
        current =  (Block *)temp_ptr;
	}
	std::cout << "|";
	if (!onlyList) {
		std::cout << std::endl << "Memory Info:\nNumber Of Blocks: " << blocks << "\nTotal Size (without Metadata): " << size << std::endl;
		std::cout << "Size of Metadata: " << sizeof(Block) << std::endl;
	}
    cout << endl;
}

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
    printf("%lx. ", (size_t)block);
    cout << (block->containing_bin == nullptr ? -1 : block->containing_bin->bin_index) << endl;
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

void checkContainingBin(){
    outi("checkContainingBin:" << endl);
    forEachBlock(assert(cur_block->containing_bin == getBinTable()+__bindex));
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

void startFunc(const char* str){
    outi("expecting: " << str << endl);
}
void endFunc(){
    outi("result:    "); printMemory(heap_start, true);
}

void allocNoFree() {
    startFunc("|U:100|U:100000|U:10|");
	void* array[4];
	array[0] = smalloc(100);
	array[1] = smalloc(100000);
	array[2] = smalloc(10);
	array[3] = smalloc(11e6);
    endFunc();
}

void alloandFree(){
    startFunc("|F:100|U:100|U:100|F:100|U:100|U:100|F:100|U:100|U:100|F:100|U:100|U:100|");
    int num = 12;
    void* arr[num];
    for(int i = 0; i < num; ++i){
        arr[i] = smalloc(100);
    }
    for(int i = 0; i < num; i += 3){
        sfree(arr[i]);
    }
    endFunc();
}

void allocandFreeMerge(){
    startFunc("|F:2525|");//2480+45
    int n = 10;
    void* arr[n];
    aref = arr;

    for(int i = 0; i < n; ++i){
        arr[i] = smalloc(100+i);
    }
    for(int i = 0; i < n; i += 2){
        sfree(arr[i]);
    }
    printTo();
    for(int i = 1; i < n; i += 2){
        outi(""); printMemory();
        sfree(arr[i]);
        printTo();
        checkContainingBin();
    }

    endFunc();
}

void mytest1(){
    printMemory(heap_start, true);
    printTo();
    void* p1 = smalloc(1);
    printMemory(heap_start, true);
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
}

int main(){
    outi("main:" << endl);
    heap_start = sbrk(0);
    
    smalloc(1300000);
    printTo();
    cout << "finished successfully\n";
}