#include "../include/uart.h"
#include "../include/io.h"

void uart_send(char c)
{
    while((readbyte(UART_LSR) & UART_LSR_EMPTY)==0);
    writebyte(c,UART_DAT);

}

void uart_send_string(char *str)
{
	int i;

	for (i = 0; str[i] != '\0'; i++)
		uart_send((char) str[i]);
}

static unsigned int uart16550_clock = 1843200;   // a common base clock
#define UART_DEFAULT_BAUD  115200

void uart_init(void)
{
	unsigned int divisor = uart16550_clock / (16 * UART_DEFAULT_BAUD);

	/* disable interrupt */
	writebyte(0, UART_IER);

	/* Enable DLAB (set baud rate divisor)*/
	writebyte(0x80, UART_LCR);
	writebyte((unsigned char)divisor, UART_DLL);
	writebyte((unsigned char)(divisor >> 8), UART_DLM);

	/*8 bits, no parity, one stop bit*/
	writebyte(0x3, UART_LCR);

	/* 使能FIFO，清空FIFO，设置14字节threshold*/
	writebyte(0xc7, UART_FCR);
}