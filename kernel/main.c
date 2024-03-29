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
#include <process.h>
#include <syscall.h>
#include <sleeplock.h>
#include <virtio.h>
#include "../usr/user_syscall.h"
#include <fat32.h>
#include <sysflags.h>
#include <elf_loader.h>
#include "../usr/shell.h"

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
	char *buf[100];
	print("this is user_thread_1\n");
    u64 fd = open("/busybox_cmd.txt",O_APPEND);
	long off = lseek(fd, 2, SEEK_SET);
	long err = read(fd, buf, 5);
	print("\n%s\n", buf);
	int a = close(fd);
	ELFExec_t * e = exec("/busybox", NULL, NULL);

	while (1)
	{
		//delay(1000);
		//print("2");
	}
	
}

int user_main()
{
	unsigned char buf[256];
	unsigned long child_stack;
	int ret;
	unsigned long i = 0;
	print("this is user_main!\n");
	child_stack = malloc(1);
	if (child_stack < 0) {
		print("cannot allocate memory\n");
		return -1;
	}

	print("malloc success 0x%x\n", child_stack);

	child_stack = malloc(1);
	if (child_stack < 0)
		print("cannot allocate memory\n");

	memset((void *)child_stack, 0, PAGE_SIZE);

	print("child_stack 0x%x\n", child_stack);
	current_dir = malloc(1);
	*current_dir = '/';
	*(current_dir + 1) = '\0';
	printf("kernel init finish!\nHELLO WORLD!\n");
	/*ret = clone(&user_thread_1,
			(void *)(child_stack + PAGE_SIZE), 0, NULL);
	if (ret < 0) {
		print("%s: error while clone\n", __func__);
		return ret;
	}*/
	/*printf("\n"
"      ▐▒▒░▄                                                                                        \n"
"       ▒▒▒▒▒▒▒▒▄                                                                                   \n"
"       ▐▒▒▒▒▒▒▒▒▒▒▒▄                                                                         ▄▄▒▒  \n"
"        ▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▄                                                               ▄▄▒▒▒▒▒▒▒▒  \n"
"        ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄                                                       ▄▒▒▒▒▒▒▒▒▒▒▒▒▒▌  \n"
"         ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄                                               ▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒   \n"
"          ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒                                        ▄░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░   \n"
"           ▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄                                ▄▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒    \n"
"             ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄▄▄▄▄░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒     \n"
"              ▀▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▀      \n"
"                ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒        \n"
"                  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒          \n"
"                    ▀▒▒▒░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▀            \n"
"                       ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▒▒▒▒▀               \n"
"                     ▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░                  \n"
"                     ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒                 \n"
"                    ▒▒▒▒▒▒▒▒▒▒░▄▄▄  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▄▄▄  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒                \n"
"                   ▒▒▒▒▒▒▒▒▒▒▒ ▀█▀   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▐██▀   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒░               \n"
"                  ▒▒▒▒▒▒▒▒▒▒▒▒       ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░       ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒               \n"
"                 ▐▒▒▒▒▒▒▒▒▒▒▒▒▒▄   ▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄   ▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒              \n"
"                 ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▀▀▀▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒              \n"
"                ▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄▄  ▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒             \n"
"                ▒▒░░░░░░░░▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░▒▒▒▒▒▒▒▒            \n"
"               ▐▒░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░▐▒▒▒▒▒▒            \n"
"               ▒▒░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▀░░░░▄░░░▀▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░▐▒▒▒▒▒▒▒           \n"
"               ▐▒▒░░░░░░░▄░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░▒▒▒▒▒▒▒▒           \n"
"                ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒▒░▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░▒▒▒▒▒▒▒▒▒▒▒▒          \n"
"                ▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒░▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒          \n"
"                 ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄░▀▒▒▒▒▀░▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒         \n"
"                  ▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒         \n"
    );*/
	while(1)
	{
		print("\n[SH]%s#:", current_dir);
		while(!keyboard.flag);
		if(!keyboard.len)
		{
			keyboard.flag = 0;
			continue;
		}
		parse_command();
	}

	/*print("clone done, 0x%lx 0x%lx\n", &user_thread_1, child_stack + PAGE_SIZE);
	while (1) {
		delay(1000);
		print("%s: %lu\n", __func__, i++);
	}*/
	//u64 fd = open("/busybox",O_RDONLY);
	//print("\n fd = %d\n",fd);
	//int a = close(fd);
	
	return 0;
}

void user_initial(void)
{
	if(create_user_place((unsigned long)&user_main))
	{
		printk("error to create user place");
	}
}



void kernel_thread()
{
	while(1)
	{
		delay(1000);
		printk("this is kernel thread!\n");
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
	enable_virtio_irq();
	printk("uart irq enable! \n");
	mem_init((unsigned long)_bss_end, ADDR_END);
	printk("ready to init_mmu!!!!!!!!!!!!\n");
	mmu_init();
	printk("mmu_ok\n");
	timer_init();
	irq_enable();
	binit();
	virtio_init();
	FAT32_init();
	int pid = do_fork(KERNEL_THREAD, user_initial, 0);
	switch_to(get_current_task(), g_task[pid]);
	while (1)
	{
		
	}
	
}
