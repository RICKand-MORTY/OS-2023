#include "../include/uart.h"
#include "../lib/printk.h"
#include "../include/trap.h"
#include "../include/sbi.h"
#include "../include/timer.h"
#include "../include/csr.h"
#include "../include/io.h"
#include "../include/plic.h"
#include <memory.h>

extern char _bss_begin[], _bss_end[];

#define PGDIR_SHIFT     30
#define PGDIR_SIZE      (1UL << PGDIR_SHIFT)
#define PGDIR_MASK      (~(PGDIR_SIZE - 1))
#define PTRS_PER_PGD	(PAGE_SIZE/ sizeof(pgd_t))
#define PAGE_SIZE   	(1 << PAGE_SHIFT)
#define PAGE_SHIFT	 		12
#define PMD_SHIFT       21
#define PTRS_PER_PMD    (1<<(PGDIR_SHIFT))
typedef unsigned long u64;
typedef u64 pteval_t;
typedef u64 pmdval_t;
typedef u64 pudval_t;
typedef u64 pgdval_t;
typedef struct {
	pgdval_t pgd;
} pgd_t;
typedef struct {
	unsigned long pgprot;
} pgprot_t;
typedef struct {
	pmdval_t pmd;
} pmd_t;
#define __pgprot(x)	((pgprot_t) { (x) })
#define PTRS_PER_PMD    (PAGE_SIZE / sizeof(pmd_t))

void kernel_main(void)
{
	uart_init();
	//uart_send_string("Welcome RISC-V!\r\n");
	trap_init();
	local_irq_enable;
	//timer_init;
	int a = TOTAL_PAGES;
	printk("%d",a);
	plic_init();
	printk("plic_init finish \n");
	enable_uart_irq();
	printk("uart irq enable! \n");
	mem_init((unsigned long)_bss_end, ADDR_END);
	u64 addr1 = page_alloc();
	u64 addr = addr1;
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	addr1 = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	unsigned long addr2 = page_alloc();
	printk("address:%016lx\n",addr);
	addr = page_alloc();
	printk("address:%016lx\n",addr);
	printk("\n");
	page_free_addr(addr1);
	printk("\n%d ", a);
	while (1)
	{
		
	};
	
}
