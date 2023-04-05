#ifndef _PAGE_TABLE_H
#define _PAGE_TABLE_H


typedef unsigned long u64; 
typedef u64 pgd_page;
typedef u64 pmd_page;    
typedef u64 pte_page;

#define PAGE_SHIFT           12
#define PAGE_SIZE		    (1UL << PAGE_SIZE)
#define PAGE_ITEMS          (PAGE_SIZE / sizeof(u64))
#define PAGE_MASK           (~(PAGE_SIZE-1))

//PGD page
#define PGD_SHIFT           30
#define PGD_SIZE            (1UL << PGD_SHIFT)
#define PGD_MASK            (~(PGD_SIZE - 1))

//PMD page
#define PMD_SHIFT       21
#define PMD_SIZE        (1UL << PMD_SHIFT)
#define PMD_MASK        (~(PMD_SIZE - 1))

//PTE page
#define PTE_SHIFT       12
#define PTE_SIZE        (1UL << PTE_SHIFT)
#define PTE_MASK        (~(PTE_SIZE-1))

//alian 
#define PAGE_ALIGN_UP(addr)         (((addr) + PAGE_SIZE - 1) & (PAGE_MASK))
#define PAGE_ALIGN_DOWN(addr)       ((addr) & (PAGE_MASK))
#endif