#include "../include/uart.h"
#include "../lib/printk.h"
#include "../include/trap.h"
#include "../include/sbi.h"
#include "../include/timer.h"
#include "../include/csr.h"
#include "../include/io.h"
#include "../include/plic.h"
#include "../lib/lib.h"
#include <memory.h>
#include <spinlock.h>

extern char _bss_begin[], _bss_end[];
extern char _text[], _etext[];
extern char _text_boot[];


void kernel_main(void)
{
	uart_init();
	uart_send_string("Welcome RISC-V!\r\n");
	trap_init();
	local_irq_enable();
	//timer_init();
	//int a = TOTAL_PAGES;
	//printk("Hello.\n");
	plic_init();
	printk("plic_init finish \n");
	enable_uart_irq();
	printk("uart irq enable! \n");
	mem_init((unsigned long)_bss_end, ADDR_END);
	printk("ready to init_mmu!!!!!!!!!!!!\n");
	mmu_init();
	//unsigned long c = (*(unsigned long*)(ADDR_END+4096));
	//printk("%016lx",c);
	printk("mmu_ok\n");
	sbi_putstr("Hello.\n");
	printk("test test\n");
	printk("This is test!\n");
	printk("\n\nHello.\n");
	printk("========== START test_write ==========\n");
    printk("Hello operating system contest.\n");
    printk("========== END test_write ==========\n\n");
	sbi_remote_SFENCE_VMA_with_ASID(0,0,0,0);
	spinlock lock;
	spin_init(&lock);
	printk("%d\n",lock.lock);
	spin_lock(&lock);
	spin_unlock(&lock);
	printk("aaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
	while(1);
}
