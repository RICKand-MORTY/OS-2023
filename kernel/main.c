#include "../include/uart.h"
#include "../lib/printk.c"
#include "../include/trap.h"
void kernel_main(void)
{
	uart_init();
	uart_send_string("Welcome RISC-V!\r\n");
	trap_init();
	printk("this is printk");
	
	__asm__("li a0,0x7000000000000\n"
			 "ld a0, (a0)"
			);
	while (1)
	{
		/* code */
	};
	
}
