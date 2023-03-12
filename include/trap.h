#ifndef _TRAP_H
#define _TRAP_H


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

#define write_csr(csr,val)						\
({												\
	__asm__ __volatile__  (						\
		"csrw "#csr", %0"						\
		:										\
		:"rK"((unsigned long)(val))				\
		:"memory"								\
	);											\
})

#define read_csr(csr)							\
({												\
	register unsigned long _res;				\
	__asm__ __volatile__(						\
		"csrr %0, " #csr						\
		:"=r"(_res)								\
		:										\
		:"memory"								\
	);											\
	_res;										\	
})

void trap_init(void);
static void do_trap_error(struct pt_regs *regs, const char *str);
void show_regs(struct pt_regs *regs);
void do_exception(struct pt_regs *regs, unsigned long scause);

#endif