#include "../../include/trap.h"
#include "../../lib/printk.h"
#include "../../include/pt_offset.h"

#define is_interrupt_fault(reg) (reg & (1UL << 63))

extern void do_exception_vector(void);

const char * error_msg[]=
{
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    "preserved 10",
    "preserved 11",
    "Instruction page fault",
    "Load page fault",
    "preserved 14",
    "Store/AMO page fault",
};

void trap_init(void)
{
	write_csr(stvec, do_exception_vector);
	printk("stvec=0x%x, 0x%x\n", read_csr(stvec), do_exception_vector);
	//inhibit all interrupts
	write_csr(sie, 0);
}

//define error process function
static void do_trap_error(struct pt_regs *regs, const char *str)
{
	printk("Oops - %s\n", str);
	show_regs(regs);
	printk("sstatus:0x%016lx  sbadaddr:0x%016lx  scause:0x%016lx\n",
			regs->sstatus, regs->sbadaddr, regs->scause);
	while(1);
}

void show_regs(struct pt_regs *regs)
{
	printk("sepc: %016lx ra : %016lx sp : %016lx\n",
		regs->sepc, regs->ra, regs->sp);
	printk(" gp : %016lx tp : %016lx t0 : %016lx\n",
		regs->gp, regs->tp, regs->t0);
	printk(" t1 : %016lx t2 : %016lx t3 : %016lx\n",
		regs->t1, regs->t2, regs->s0);
	printk(" s1 : %016lx a0 : %016lx a1 : %016lx\n",
		regs->s1, regs->a0, regs->a1);
	printk(" a2 : %016lx a3 : %016lx a4 : %016lx\n",
		regs->a2, regs->a3, regs->a4);
	printk(" a5 : %016lx a6 : %016lx a7 : %016lx\n",
		regs->a5, regs->a6, regs->a7);
	printk(" s2 : %016lx s3 : %016lx s4 : %016lx\n",
		regs->s2, regs->s3, regs->s4);
	printk(" s5 : %016lx s6 : %016lx s7 : %016lx\n",
		regs->s5, regs->s6, regs->s7);
	printk(" s8 : %016lx s9 : %016lx s10: %016lx\n",
		regs->s8, regs->s9, regs->s10);
	printk(" s11: %016lx t3 : %016lx t4: %016lx\n",
		regs->s11, regs->t3, regs->t4);
	printk(" t5 : %016lx t6 : %016lx\n",
		regs->t5, regs->t6);
}

void do_exception(struct pt_regs *regs, unsigned long scause)
{
    int error_index=0;
	printk("%s, scause:0x%lx\n", __func__, scause);

	if (is_interrupt_fault(scause)) 
    {
        //do interrupt function
	} 
    else
    {
        error_index = (scause&0xf);
        do_trap_error(regs,error_msg[error_index]);
	}
}

