#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9') 

char buf[4096]={0};

#define do_div(n,base)({ \
unsigned int __base = (base); \
unsigned int __rem; \
__rem = ((unsigned long)(n)) % __base; \
(n) = ((unsigned long)(n)) / __base; \
__rem;\
})

int skip_atoi(const char **s);
static char * number(char * str, long num, int base, int size, int precision,	int type);
int vsprintf(char * buf,const char *fmt, va_list args);
int printk(const char *fmt,...);
int print_f(const char *fmt,...);

#endif