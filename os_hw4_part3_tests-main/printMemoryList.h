//
// Created by ariel on 25/06/2021.
//

#ifndef OS_WET4_PRINTMEMORYLIST_H
#define OS_WET4_PRINTMEMORYLIST_H

//void printMemory1(void* start);
//void printMemory2(void* start);
//void printMemory3(void* start);
//void printMemory4(void* start);

#include <iostream>
#include "unistd.h"

typedef struct stats_t {
	size_t num_free_blocks = 0;
	size_t num_free_bytes = 0;
	size_t num_allocated_blocks = 0;
	size_t num_allocated_bytes = 0;
	size_t num_meta_data_bytes = 0;
} stats;


template<class T>
void printMemory(void *start, bool onlyList) {
	T *current = (T *) start;
	size_t size = 0;
	int blocks = 0;
	if (!onlyList) {
		std::cout << "Printing Memory List\n";
	}
    T * program_break = (T*)sbrk(0);
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
        temp_ptr += effective_size + sizeof (T);
        current =  (T *)temp_ptr;
	}
	std::cout << "|";
	if (!onlyList) {
		std::cout << std::endl << "Memory Info:\nNumber Of Blocks: " << blocks << "\nTotal Size (without Metadata): " << size << std::endl;
		std::cout << "Size of Metadata: " << sizeof(T) << std::endl;
	}
}

void resetStats(stats &current_stats) {
	current_stats.num_allocated_blocks = 0;
	current_stats.num_allocated_bytes = 0;
	current_stats.num_free_blocks = 0;
	current_stats.num_free_bytes = 0;
	current_stats.num_meta_data_bytes = 0;
}


template<class T>
void updateStats(void *start, stats &current_stats, size_t bytes_mmap, int blocks_mmap) {
	resetStats(current_stats);
	T *current = (T *) start;
	T * program_break = (T*)sbrk(0);
	while (current != program_break) {
		current_stats.num_meta_data_bytes += sizeof(T);
		current_stats.num_allocated_bytes += current->udata_size;
		current_stats.num_allocated_blocks++;
		if (current->is_free) {
			current_stats.num_free_blocks++;
			current_stats.num_free_bytes += current->udata_size;
		}
        size_t effective_size = current->udata_size;
        char * temp_ptr = (char *)current;
        temp_ptr += effective_size + sizeof (T);
        current =  (T *)temp_ptr;//		current = current->next;
	}
	current_stats.num_meta_data_bytes += sizeof(T) * blocks_mmap;
	current_stats.num_allocated_bytes += bytes_mmap;
	current_stats.num_allocated_blocks += blocks_mmap;
}


#endif //OS_WET4_PRINTMEMORYLIST_H
