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
#include <process.h>

extern char _bss_begin[], _bss_end[];
extern char _text[], _etext[];
extern char _text_boot[];
void _start(void);
void _hart_start(void);
typedef struct sbiret {
  long error;
  long value;
}error;


spinlock print_lock = {.lock = 0};

void thread_1()
{
   while (1)
   {	
		delay(1000);
		printk("this is qwk\n");
   }
}

void thread_2()
{
    while (1)
    {
		delay(1000);
		printk("this is hc\n");
    }
}

void thread_3()
{
    while (1)
    {
		delay(1000);
		printk("this is lsj\n");
    }
}

void kernel_main(void)
{
	uart_init();
	uart_send_string("Welcome RISC-V!\r\n");
	sched_init();
	trap_init();
	local_irq_enable();
	plic_init();
	printk("plic_init finish \n");
	enable_uart_irq();
	printk("uart irq enable! \n");
	mem_init((unsigned long)_bss_end, ADDR_END);
	printk("ready to init_mmu!!!!!!!!!!!!\n");
	mmu_init();
	printk("mmu_ok\n");
	timer_init();
	//printk("\n\nHello.\n");
	//printk("========== START test_write ==========\n");
    //printk("Hello operating system contest.\n");
    //printk("========== END test_write ==========\n\n");
	int pid1 = do_fork(KERNEL_THREAD, thread_1, NULL);
	int pid2 = do_fork(KERNEL_THREAD, thread_2, NULL);
	int pid3 = do_fork(KERNEL_THREAD, thread_3, NULL);
	switch_to(get_current_task(),g_task[pid1]);
	while(1);
}
