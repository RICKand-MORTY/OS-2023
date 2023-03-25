#include "../../include/sbi.h"
#include "../../include/io.h"
#include "../../include/trap.h"
#include "../../include/csr.h"
#include "../../lib/printk.h"

#define TIMER_FREQ 10000000
#define HZ 1000

#define current_timer_cycles        \ 
({                                  \
    unsigned long _time;            \
    __asm__ volatile (              \
        "csrr %0, time"             \
        :"=r"(_time)                \
        :                           \
        :"memory"                   \
    );                              \
    _time;                          \
})

void reset_timer()
{
    //stack_trace();
    //printk("%ld",current_timer_cycles);
    sbi_set_timer(current_timer_cycles+TIMER_FREQ/HZ);
    csr_set(sie, SIE_STIE);     //enable timer interrupt
}

