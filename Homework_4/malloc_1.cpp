#include <unistd.h>

using namespace std;

/**
 * @brief Tries to allocate 'size' bytes.
 * @return
 *  success - a pointer to the first allocated byte within the allocated block.
 *  failure -
 *      if 'size' is 0, or more than 100,000,000 - return NULL.
 *      if 'sbrk()' fail - return NULL.
 */
void* smalloc(size_t size){
    if(size == 0 || size > 100000000)
        return NULL;    
    
    void* prev_break = sbrk(size);
    return prev_break == (void*)-1 ? NULL : prev_break;
}