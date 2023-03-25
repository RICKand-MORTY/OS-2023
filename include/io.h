#ifndef _P_IO_H
#define _P_IO_H

#define __arch_getword(a)			(*(volatile unsigned int *)(a))
#define __arch_putword(v,a)		(*(volatile unsigned int *)(a) = (v))

#define __arch_getbyte(a)			(*(volatile unsigned char *)(a))
#define __arch_putbyte(v,a)		(*(volatile unsigned char *)(a) = (v))

#define __arch_getqword(a)			(*(volatile unsigned long *)(a))
#define __arch_putqword(v,a)		(*(volatile unsigned long *)(a) = (v))

#define refresh_cache()		__asm__ __volatile__ ("" : : : "memory")

#define readword(c)	({ unsigned int  __v = __arch_getword(c); refresh_cache(); __v; })
#define writeword(v,c)	({ unsigned int  __v = v; refresh_cache(); __arch_putword(__v,c);})

#define readbyte(c)	({ unsigned char  __v = __arch_getbyte(c); refresh_cache(); __v; })
#define writebyte(v,c)	({ unsigned char  __v = v; refresh_cache(); __arch_putbyte(__v,c);})

#define readqword(c)	({ unsigned long  __v = __arch_getqword(c); refresh_cache(); __v; })
#define writeqword(v,c)	({ unsigned long  __v = v; refresh_cache(); __arch_putqword(__v,c);})

#endif