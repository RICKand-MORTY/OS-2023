#ifndef _MEMORY_H
#define _MEMORY_H


#include "page_table.h"

#define RAM_ADDR    0x80000000UL
#define TOTAL_MEM   0x8000000UL       /*128M(see makefile)*/

#define TOTAL_PAGES         (TOTAL_MEM / PAGE_SIZE)
#define ADDR_END            (RAM_ADDR + TOTAL_MEM)


void mem_init(u64 mem_start, u64 mem_end);
unsigned long page_alloc();
int page_free(unsigned int count);
int page_free_addr(unsigned long addr);

#endif