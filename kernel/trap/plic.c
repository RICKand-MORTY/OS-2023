#include "../../include/plic.h"
#include "../../include/io.h"
#include "../../include/csr.h"
#include "../../include/uart.h"


//set prority of plic interrupt number
void plic_set_prority(int irq_num, int pri)
{
    unsigned int reg = PLIC_PRIORITY_REG(irq_num);
    writeword(pri, reg);
}

//open or disable PLIC irq
void plic_switch_irq(int core, int irq_num, bool on_off)
{
    //printk("%s bgein!\n",__func__);
    unsigned int mask = 1 << (irq_num % 32);
    unsigned int hart = CPU_TO_HART(core);      
    unsigned int reg = PLIC_M_INTENABLE_REG(hart) + ((irq_num >> 5) ? 4 : 0);
    printk("reg = %016lx,   irq_num = %016lx\n",reg, irq_num);
    if(on_off)
    {
        writeword(readword(reg) | mask, reg);
    }
    else
    {
        writeword(readword(reg) & ~mask, reg);
    }
    //printk("%s end!\n",__func__);
}

//plic interrupt handle function
void handle_plic_irq(struct pt_regs *regs)
{
    int irq_num = 0;
    int hart = CPU_TO_HART(0); 
    unsigned int reg = PLIC_M_CLAIM_REG(hart);
    csr_clr(sie, SIE_SEIE);
    while((irq_num = readword(reg)))
    {
        if(irq_num == UART0_IRQ_NUM)
        {
            handle_uart_irq();
        }
        writeword(irq_num, reg);
    }
    csr_set(sie, SIE_SEIE);
}

//uart handle function
void handle_uart_irq()
{
    char c = uart_get();
    if(c < 0)
    {
        return;
    }
    else if (c == '\r')
    {
        printk("uart interrupt!%s\n", __func__);
    }
    else
    {
        uart_send(c);
    }
}

int plic_init()
{
    int i;
    int irq_num;
    for(i = 0;i < U7_CORE;i++)
    {
        writeword(0, PLIC_M_THRESHOLD_REG(CPU_TO_HART(i))); //set priority threshold to 0 of M mode
        for(irq_num = 1; irq_num <= MAX_PLIC_IRQ ; irq_num++)
        {
            //printk("%s\n",__func__);
            //close all external interrupt
            plic_switch_irq(i, irq_num, 0);
            //set all priority of irq_num to 1
            plic_set_prority(irq_num, 1);
        }
    }
    csr_set(sie, SIE_SEIE);     //open external interrupt of S mode
    return 0;
}

