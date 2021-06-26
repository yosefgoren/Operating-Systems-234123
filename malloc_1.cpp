#include "malloc_1.h"
#include <iostream>

using namespace std;

void* smalloc(size_t size){
    if(size == 0 || size > 100000000)
        return NULL;    
    
    void* prev_break = sbrk(size);
    return prev_break == (void*)-1 ? NULL : prev_break;
}