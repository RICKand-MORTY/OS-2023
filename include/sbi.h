//used for syscall
#ifndef _SBI_H
#define _SBI_H

#define SBI_CALL(__EID, __arg0, __arg1, __arg2)                         \
({                                                                      \
    register unsigned long a0 asm ("a0") = (unsigned long)(__arg0);     \
    register unsigned long a1 asm ("a1") = (unsigned long)(__arg1);     \
    register unsigned long a2 asm ("a2") = (unsigned long)(__arg2);     \
    register unsigned long a7 asm ("a7") = (unsigned long)(__EID);      \
    __asm__ volatile (                                                  \
        "ecall"                                                         \
        :"+r"(a0)                                                       \
        :"r"(a1), "r"(a2), "r"(a7)                                      \
        :"memory"                                                       \
    );                                                                  \
    a0;                                                                 \
})

#define SBI_CALL_0(EID) SBI_CALL(EID, 0, 0, 0)
#define SBI_CALL_1(EID, arg0) SBI_CALL(EID, arg0, 0, 0)
#define SBI_CALL_2(EID, arg0, arg1) SBI_CALL(EID, arg0, arg1, 0)

#define SBI_SET_TIMER 0x00
#define SBI_CONSOLE_PUTCHAR 0X01
#define SBI_CONSOLE_GETCHAR 0X02

#define sbi_set_timer(time) do {SBI_CALL_1(SBI_SET_TIMER, time);} while(0)
#define sbi_putchar(s) do {SBI_CALL_1(SBI_CONSOLE_PUTCHAR, s);} while(0)

static inline void sbi_putstr(char *str)
{
    int i=0;
    for(i=0;str[i]!='\0';i++)
    {
        sbi_putchar(str[i]);
    }
}

#endif