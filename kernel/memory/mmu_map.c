#include <page_table.h>
#include "../../lib/printk.h"
#include <memory.h>
#include "../../lib/lib.h"



unsigned long alloc_pgtable()
{
    unsigned long addr = page_alloc();
    if(addr == 1)
    {
        printk("alloc_pgtable fail!\n");
        return 1;
    }
    memset((void*)addr, 0, PAGE_SIZE);
    return addr;
}


pmd_page* get_pmd_from_pgd(pgd_page* pgd_ptr,u64 va)
{
    pmd_page *pmd_base;
    u64 index = PMD_INDEX(va);
    pmd_base = (pmd_page*)PMD_BASE(*pgd_ptr);
    return pmd_base + index;

}

pmd_page* get_pte_from_pmd(pmd_page* pgd_ptr,u64 va)
{
    pte_page *pte_base;
    u64 index = PTE_INDEX(va);
    pte_base = (pte_page*)PTE_BASE(*pgd_ptr);
    return pte_base + index;

} 

void pt_pgd_item(pgd_page* pgd_ptr, pgd_page pgd, u64 attr)
{
    pgd = pgd >> PAGE_SHIFT << PT_PPN;
    pgd |= attr;
    *pgd_ptr = pgd;
}

void create_identity_mapping()
{
    //mapping _text_boot and _etext;
    u64 start = (unsigned long)_text_boot;
    u64 end = (unsigned long )_etext;
    create_pgd_mapping((pgd_page*)_pgd_page, start, start, end - start, PAGE_KERNEL_READ_EXEC, alloc_pgtable, 0);
    printk("_text_boot and _etext map done!\n");
    
    //mapping _etext and ADDR_END
    start = PAGE_ALIGN_UP((unsigned long)_etext);
    end = ADDR_END;
    create_pgd_mapping((pgd_page*)_pgd_page, start, start, end - start, PAGE_KERNEL, alloc_pgtable, 0);
    printk("_etext map done!\n");
}

void create_VIRT_UART_mapping()
{
    u64 start = CLINT_ADDR;
    create_pgd_mapping((pgd_page*)_pgd_page, start, start, CLINT_SIZE, PAGE_KERNEL, alloc_pgtable, 0);
    start = UART_ADDR;
    create_pgd_mapping((pgd_page*)_pgd_page, start, start, UART_SIZE, PAGE_KERNEL, alloc_pgtable, 0);
}
void create_pgd_mapping(
    pgd_page* pgd_base,             /*PGD page base address(in satp PNN)*/
    u64 phy_addr,                   /*The starting address of the physical memory to be mapped*/
    u64 virt_addr,                  /*The starting address of the virtual address*/
    u64 size,                       /*Total size of mapping address*/
    u64 attr,                       /*page attribute*/
    u64 (*alloc_func)(void),        /*alloc function of next level of page table*/
    u64 flags                       /*flags when creating page table*/
)
{
    u64 next_pgd=0;
    u64 addr = virt_addr & PAGE_MASK;
    u64 end = PAGE_ALIGN_UP(virt_addr + size);
    pgd_page* pgd_ptr = PGD_OFFSET(pgd_base, virt_addr);
    phy_addr &= PAGE_MASK;
    do
    {
        next_pgd = NEXT_PGD(addr, end);
        create_pmd_mapping(pgd_ptr, addr, next_pgd, phy_addr, attr, alloc_pgtable, flags);
        phy_addr += next_pgd - addr;
        addr = next_pgd;
        pgd_ptr++;
    } while (addr != end);
}

void create_pmd_mapping(
    pgd_page* pgd_base,
    u64 addr,             
    u64 end,                   
    u64 phy_addr,                                       
    u64 attr,                       
    u64 (*alloc_func)(void),        
    u64 flags                  
)
{
    pgd_page pgd = *pgd_base;
    pmd_page *pmd_ptr = NULL;
    pmd_page new_pmd = 0;
    u64 next_pmd = 0;
    if(pgd == 0)
    {
        printk("this is create_pmd_mapping!\n");
        new_pmd = alloc_pgtable();
        if(new_pmd == 1)
        {
            printk("create_pmd_mapping fail! addr = %016lx, pgd_base = %016lx\n", addr, pgd_base);
            return;
        }
        else
        {
            FILL_PGD_ITEM(pgd_base, new_pmd, PAGE_ATTR_VALID);
            pgd = *pgd_base;
        }
    }
    pmd_ptr = get_pmd_from_pgd(pgd_base, addr);
    do
    {
        next_pmd = NEXT_PMD(addr, end);
        create_pte_mapping(pmd_ptr, addr, next_pmd, phy_addr, attr, alloc_pgtable, flags);
        phy_addr += next_pmd - addr;
        addr = next_pmd;
        pmd_ptr++;
    } while (addr != end);
}

void create_pte_mapping(
    pmd_page* pmd_base,
    u64 addr,             
    u64 end,                   
    u64 phy_addr,                                       
    u64 attr,                       
    u64 (*alloc_func)(void),        
    u64 flags                  
)
{
    //printk("this is create_pte_mapping!!!!!!!!!!!!!!!!!!\n");
    pmd_page pmd = *pmd_base;
    pte_page* pte_ptr = NULL;
    pmd_page new_pte = 0;
    if(pmd == 0)
    {
        new_pte = alloc_pgtable();
        if(new_pte == 1)
        {
            //printk("create_pte_mapping fail! addr = %016lx, pmd_base = %016lx\n", addr, pmd_base);
            return;
        }
        else
        {
            FILL_PMD_ITEM(pmd_base, new_pte, PAGE_ATTR_VALID);
            pmd =*pmd_base;
        }
    }
    pte_ptr = get_pte_from_pmd(pmd_base, addr);
    do
    {
        FILL_PTE_ITEM(pte_ptr, phy_addr, attr);
        phy_addr += PAGE_SIZE;
        addr += PAGE_SIZE;
        pte_ptr++;
    }while(addr != end);
}

/*equally mapping*/
void mmu_init(void)
{
    memset(_pgd_page, 0, PAGE_SIZE);
    create_identity_mapping();
    create_VIRT_UART_mapping();
    enable_mmu();
    //printk("mmu init done!!!!!!!!!!!!!!!!!!\n");
}