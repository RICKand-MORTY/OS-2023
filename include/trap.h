#ifndef _TRAP_H
#define _TRAP_H


//syscall
#define EXC_SYSCALL		8

//CLINT register (unmapped)
#define CLINT_ADDR	0x2000000
#define CLINT_TIMER_CMP (CLINT_ADDR + 0x4000)
#define CLINT_TIMER_VAL (CLINT_ADDR + 0xbff8)

//CLINT interrupt num
#define S_INTERRUPT_CAUSE_SOFTWARE 1
#define S_INTERRUPT_CAUSE_TIMER 5
#define S_INTERRUPT_CAUSE_EXTERNAL 9

/*store registers when exception happened*/
struct pt_regs {
	unsigned long sepc;
	unsigned long ra;
	unsigned long sp;
	unsigned long gp;
	unsigned long tp;
	unsigned long t0;
	unsigned long t1;
	unsigned long t2;
	unsigned long s0;
	unsigned long s1;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
	unsigned long s2;
	unsigned long s3;
	unsigned long s4;
	unsigned long s5;
	unsigned long s6;
	unsigned long s7;
	unsigned long s8;
	unsigned long s9;
	unsigned long s10;
	unsigned long s11;
	unsigned long t3;
	unsigned long t4;
	unsigned long t5;
	unsigned long t6;
	/* Supervisor CSRs */
	unsigned long sstatus;
	unsigned long sbadaddr;
	unsigned long scause;
};


void trap_init(void);
static void do_trap_error(struct pt_regs *regs, const char *str);
void show_regs(struct pt_regs *regs);
void do_exception(struct pt_regs *regs, unsigned long scause);
void stack_trace(void);
#endif