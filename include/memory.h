#ifndef _MEMORY_H
#define _MEMORY_H


#include "page_table.h"
#include <trap.h>

extern char _text_boot[], _etext_boot[];
extern char _text[], _etext[];
extern char _pgd_page_begin[], _pgd_page_end[];

#define BITMAPS_SIZE ((TOTAL_PAGES / (sizeof(u64) * 8)) + 1)
#define RAM_ADDR    0x80000000UL
#define TOTAL_MEM   0x8000000UL       /*128M(see makefile)*/

#define TOTAL_PAGES         (TOTAL_MEM / PAGE_SIZE)
#define ADDR_END            (RAM_ADDR + TOTAL_MEM)

#define UART_ADDR        0x10000000UL
#define CLINT_SIZE	     0x10000UL
#define UART_SIZE        4096

typedef unsigned long u64; 
typedef u64 pgd_page;
typedef u64 pmd_page;    
typedef u64 pte_page;

//page_alloc.c
void mem_init(u64 mem_start, u64 mem_end);
unsigned long page_alloc();
int page_free(unsigned int count);
int page_free_addr(unsigned long addr);
unsigned long more_page_alloc(int count);
int more_page_free(void *buf, unsigned int count);

//mmu_map.c
unsigned long alloc_pgtable();
void mmu_init(void);
pmd_page* get_pmd_from_pgd(pgd_page* pgd_ptr,u64 va);
pmd_page* get_pte_from_pmd(pmd_page* pgd_ptr,u64 va);
void pt_pgd_item(pgd_page* pgd_ptr, pgd_page pgd, u64 attr);
void create_identity_mapping();
void create_pgd_mapping(
    pgd_page* pgd_base,             /*PGD page base address(in satp PNN)*/
    u64 phy_addr,                   /*The starting address of the physical memory to be mapped*/
    u64 virt_addr,                  /*The starting address of the virtual address*/
    u64 size,                       /*Total size of mapping address*/
    u64 attr,                       /*page attribute*/
    u64 (*alloc_func)(void),        /*alloc function of next level of page table*/
    u64 flags                       /*flags when creating page table*/
);
void create_pmd_mapping(
    pgd_page* pgd_base,
    u64 addr,             
    u64 end,                   
    u64 phy_addr,                                       
    u64 attr,                       
    u64 (*alloc_func)(void),        
    u64 flags                  
);
void create_pte_mapping(
    pmd_page* pmd_base,
    u64 addr,             
    u64 end,                   
    u64 phy_addr,                                       
    u64 attr,                       
    u64 (*alloc_func)(void),        
    u64 flags                  
);
extern unsigned long pages_bitmaps[BITMAPS_SIZE];
#endif