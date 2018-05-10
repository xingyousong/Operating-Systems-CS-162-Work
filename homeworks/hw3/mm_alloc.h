/*
 * mm_alloc.h
 *
 * A clone of the interface documented in "man 3 malloc".
 */

#pragma once

#include <stdlib.h>

#define META_SIZE 0x20

void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);
void mm_free(void *ptr);


struct mem_block{
	size_t size;
	struct mem_block *next;
	struct mem_block *prev; 
	int free;
	char* actual_memory[0];
};

typedef struct mem_block *mem_ptr; 


mem_ptr combine(mem_ptr temp);
void split(mem_ptr start, size_t s);
mem_ptr increase_mem(mem_ptr start, size_t size);