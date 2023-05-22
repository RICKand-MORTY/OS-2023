#include <spinlock.h>
#include "../../lib/printk.h"
#include <csr.h>


//initial spinlock
void spin_init(spinlock_P lock)
{
    lock->lock = 0;     //0 is unlock,1 is lock
}

//acquire spinlock
void spin_lock(spinlock_P lock)
{
    csr_clr(sie, SIE_SEIE);      //need to disable interrupt to avoid dead lock
    register unsigned long a1 asm ("a1") = &(lock->lock); 
    __asm__ __volatile__ (
        "1:# get_lock\n"
        "li a2, 1\n"
        "2:# retry\n"
        "amomax.w a3, a2, (%0)\n"
        "bnez a3, 2b\n"
        :"+r"(a1)
        :
        :"a2", "a3", "memory"
    );
    csr_set(sie, SIE_SEIE);     //enable timer interrupt
    printk("%d\n",lock->lock);
}

//release spinlock
void spin_unlock(spinlock_P lock)
{
    csr_clr(sie, SIE_SEIE);
    register unsigned long a1 asm ("a1") = &(lock->lock);
    __asm__ __volatile__(
        "sw x0, (%0)\n"
        :"+r"(a1)
        :
        :"memory"
    );
    csr_set(sie, SIE_SEIE);
    printk("%d\n",lock->lock);
}