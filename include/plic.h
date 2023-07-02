#ifndef _PLIC_H
#define _PLIC_H
#include <stdbool.h>
#include "trap.h"

//PLIC registers address
#define MAX_PLIC_IRQS             53
#define PLIC_BASE_ADDR            0xc000000UL
#define PLIC_PENDING_BASE         PLIC_BASE_ADDR + 0x1000
#define PLIC_M_INTENABLE_BASE     PLIC_BASE_ADDR + 0x2000
#define PLIC_M_THRESHOLD_BASE     PLIC_BASE_ADDR + 0x200000
#define PLIC_M_CLAIM_BASE         PLIC_BASE_ADDR + 0x200004

#define PLIC_PRIORITY_REG(irq_num)    (PLIC_BASE_ADDR + (irq_num) * 4)
#define PLIC_PENDING_REG(irq_num)     ((irq_num >> 5 == 0) ? (PLIC_PENDING_BASE):(PLIC_PENDING_BASE + 0x04))
#define PLIC_M_INTENABLE_REG(hart)      ( PLIC_M_INTENABLE_BASE + (hart) * 0x80)
#define PLIC_M_THRESHOLD_REG(hart)    (PLIC_M_THRESHOLD_BASE + (hart) * 0x1000)
#define PLIC_M_CLAIM_REG(hart)        (PLIC_M_CLAIM_BASE + (hart) * 0x1000)    

#define MAX_PLIC_IRQ 53
#define U7_CORE 1
#define CPU_TO_HART(core) ((core)+1)    //U7 core to hart,skip out S7 core

void plic_set_prority(int irq_num, int pri);
void plic_switch_irq(int core, int irq_num, bool on_off);
void handle_plic_irq(struct pt_regs *regs);
int plic_init();
int handle_uart_irq();
void enable_virtio_irq();
#endif