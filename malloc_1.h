#ifndef MALLOC_1
#define MALLOC_1

#include "utils.h"

/**
 * @brief Tries to allocate 'size' bytes.
 * @return
 *  success - a pointer to the first allocated byte within the allocated block.
 *  failure -
 *      if 'size' is 0, or more than 100,000,000 - return NULL.
 *      if 'sbrk()' fail - return NULL.
 */
void* smalloc(size_t size);

#endif