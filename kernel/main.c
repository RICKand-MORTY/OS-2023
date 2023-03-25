#include "../include/uart.h"
#include "../lib/printk.h"
#include "../include/trap.h"
#include "../include/sbi.h"
#include "../include/timer.h"
#include "../include/csr.h"
#include "../include/io.h"

void kernel_main(void)
{
	uart_init();
	//uart_send_string("Welcome RISC-V!\r\n");
	trap_init();
	//sbi_putstr("this is sbi print\n");
	//printk("abcdefg\n%daaa",(1+2));
	/*__asm__("li a0,0x7000000000000\n"
			 "ld a0, (a0)"
			);*/
	timer_init;
	local_irq_enable;
	while (1)
	{
		
	};
	
}
