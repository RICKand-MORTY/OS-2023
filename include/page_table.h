#ifndef _PAGE_TABLE_H
#define _PAGE_TABLE_H

#define NULL 0

typedef unsigned long u64; 
typedef u64 pgd_page;
typedef u64 pmd_page;    
typedef u64 pte_page;

#define PAGE_SHIFT           12
#define PAGE_SIZE		    (1UL << PAGE_SHIFT)
#define PAGE_ITEMS          (PAGE_SIZE / sizeof(u64))
#define PAGE_MASK           (~(PAGE_SIZE-1))
#define PT_PPN               10

//PGD page
#define PGD_SHIFT           30
#define PGD_SIZE            (1UL << PGD_SHIFT)
#define PGD_MASK            (~(PGD_SIZE - 1))

//PMD page
#define PMD_SHIFT           21
#define PMD_SIZE            (1UL << PMD_SHIFT)
#define PMD_MASK            (~(PMD_SIZE - 1))

//PTE page
#define PTE_SHIFT           12
#define PTE_SIZE            (1UL << PTE_SHIFT)
#define PTE_MASK            (~(PTE_SIZE-1))

//align 
#define PAGE_ALIGN_UP(addr)         (((addr) + PAGE_SIZE - 1) & (PAGE_MASK))
#define PAGE_ALIGN_DOWN(addr)       ((addr) & (PAGE_MASK))

//page attribute
#define PAGE_ATTR_VALID         (1 << 0)
#define PAGE_ATTR_READ          (1 << 1)
#define PAGE_ATTR_WRITE         (1 << 2)
#define PAGE_ATTR_EXEC          (1 << 3)
#define PAGE_ATTR_USER          (1 << 4)
#define PAGE_ATTR_GLOBAL        (1 << 5)
#define PAGE_ATTR_ACCESS        (1 << 6)
#define PAGE_ATTR_DIRTY         (1 << 7)

#define PAGE_BASE               (PAGE_ATTR_VALID | PAGE_ATTR_ACCESS | PAGE_ATTR_USER)

//kernel page attribute
#define PAGE_KERNEL             (PAGE_ATTR_VALID | PAGE_ATTR_READ | PAGE_ATTR_WRITE | PAGE_ATTR_ACCESS | PAGE_ATTR_DIRTY | PAGE_ATTR_GLOBAL)
#define PAGE_KERNEL_READ        (PAGE_KERNEL & ~(PAGE_ATTR_WRITE))
#define PAGE_KERNEL_READ_EXEC   (PAGE_KERNEL_READ | PAGE_ATTR_EXEC)
#define PAGE_KERNEL_EXEC        (PAGE_KERNEL | PAGE_ATTR_EXEC)

//find index of page table
#define PGD_INDEX(addr)                 (((addr) >> (PGD_SHIFT)) & (PAGE_ITEMS - 1))
#define PGD_OFFSET(pgd_base, addr)      ((pgd_base) + PGD_INDEX(addr))

#define NEXT_PGD(addr, end)                                               \
({                                                                        \
    unsigned long next_page = ((addr) + PGD_SIZE) & PGD_MASK;             \
    (next_page < (end)) ? next_page : (end);                              \
})

#define PMD_INDEX(addr)                 (((addr) >> (PMD_SHIFT)) & (PAGE_ITEMS - 1))
#define PMD_BASE(pgd)                 ((((pgd) >> PT_PPN) << PAGE_SHIFT))
#define NEXT_PMD(addr, end)                                               \
({                                                                        \
    unsigned long next_page = ((addr) + PMD_SIZE) & PMD_MASK;             \
    (next_page < (end)) ? next_page : (end);                              \
})


#define PTE_INDEX(addr)               (((addr) >> (PAGE_SHIFT)) & (PAGE_ITEMS - 1))
#define PTE_BASE(pmd)                 ((((pmd) >> PT_PPN) << PAGE_SHIFT))  

//fill the pgd/pmd to the PNN in pgd item
#define FILL_PGD_ITEM(pgd_ptr, pgd, attr) do {(pgd) = ((pgd) >> PAGE_SHIFT << PT_PPN); (pgd) |= (attr); (*pgd_ptr) = pgd;} while(0)
#define FILL_PMD_ITEM(pmd_ptr, pmd, attr) do {(pmd) = (pmd) >> PAGE_SHIFT << PT_PPN; (pmd) |= (attr); (*pmd_ptr) = pmd;} while(0)
#define FILL_PTE_ITEM(pte_ptr, pte, attr) do {(pte) = (pte) >> PAGE_SHIFT << PT_PPN; (pte) |= (attr); (*pte_ptr) = pte;} while(0)

#endif