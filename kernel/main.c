#include "../include/uart.h"
#include "../lib/printk.c"
void kernel_main(void)
{
	int i=1;
	uart_init();
	uart_send_string("Welcome RISC-V!\r\n");
	printk("hello!!!%d\n",i);
	stack_trace();
	while (1) {
		;
	}
}
