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
#include <syscall.h>
#include <virtio.h>
#include "../usr/user_syscall.h"

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

int user_thread_1(void *arg)
{
    while (1)
    {
		sleep(1000);
		print("this is hc\n");
    }
}

int user_main()
{
	unsigned long child_stack;
	int ret;
	unsigned long i = 0;

	child_stack = malloc();
	if (child_stack < 0) {
		print("cannot allocate memory\n");
		return -1;
	}

	print("malloc success 0x%x\n", child_stack);

	child_stack = malloc();
	if (child_stack < 0)
		print("cannot allocate memory\n");

	memset((void *)child_stack, 0, PAGE_SIZE);

	print("child_stack 0x%x\n", child_stack);

	ret = clone(&user_thread_1,
			(void *)(child_stack + PAGE_SIZE), 0, NULL);
	if (ret < 0) {
		print("%s: error while clone\n", __func__);
		return ret;
	}

	print("clone done, 0x%llx 0x%llx\n", &user_thread_1, child_stack + PAGE_SIZE);
	while (1) {
		sleep(1000);
		print("%s: %lu\n", __func__, i++);
	}

	return 0;
}

void user_initial(void)
{
	if(create_user_place((unsigned long)&user_main))
	{
		printk("error to create user place");
	}
}

void thread_1()
{
   while (1)
   {	
		delay(1000);
		printk("this is qwk\n");
   }
}


void user_thread_2()
{
    while (1)
    {
		delay(1000);
		printk("this is hc\n");
    }
}

void user_thread_3()
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
	//int pid1 = do_fork(KERNEL_THREAD, thread_1, NULL);
	//int pid2 = do_fork(KERNEL_THREAD, thread_2, NULL);
	//int pid3 = do_fork(KERNEL_THREAD, thread_3, NULL);
	//switch_to(get_current_task(),g_task[pid1]);
	PCB p;
	printk("NEED_SCHEDULE_OFFSET=%ld\n",( (unsigned long)&p.need_schedule - (unsigned long)&p));
	printk("SCRAMBLE_OFFSET=%ld\n",( (unsigned long)&p.scramble -  (unsigned long)&p));
	printk("KERNEL_SP_OFFSET=%ld\n",( (unsigned long)&p.kernel_sp - (unsigned long) &p));
	printk("USER_SP_OFFSET=%ld\n",( (unsigned long)&p.user_sp -  (unsigned long)&p));
	printk("CONTEXT_OFFSET=%ld\n",( (unsigned long)&p.context -  (unsigned long)&p));
	//int pid = do_fork(KERNEL_THREAD, user_initial, 0);
	//switch_to(get_current_task(), g_task[pid]);
	printk("virtio_regs size = %d\n",sizeof(virtio_regs));
	irq_enable();
	virtio_init();
	unsigned char buff[512];
	//virtio_rw(1, &buff, VIRTIO_BLK_T_IN);
	virtio_rw(VIRTIO_BLK_T_IN, 1, buff);
	while(1);
}
