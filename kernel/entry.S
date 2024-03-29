#include "../include/pt_offset.h"
#include <scheduler.h>
//sie register
#define SIE_SSIE 0X02UL     //software interrupt
#define SIE_STIE 0x20UL     //timer interrupt
#define SIE_SEIE 0x200UL    //external interrupt

//sstatus register 
#define SSTATUS_SIE 0X02UL  //enable S mode interrupt
#define SSTATUS_SPIE 0x20UL      //previous interrupt status
#define SSTATUS_SPP 0x100UL      //Mode before interrupt

.macro kernel_entry

	csrrw tp, sscratch, tp
	bnez tp, _save_user_sp
_save_kernel_sp:
	csrr tp, sscratch
	sd sp, KERNEL_SP_OFFSET(tp)
_save_user_sp:
	sd sp, USER_SP_OFFSET(tp)
	ld sp, KERNEL_SP_OFFSET(tp)

	addi sp, sp, -(PT_SIZE)
	sd x1,  PT_RA(sp)
	sd x3,  PT_GP(sp)
	sd x5,  PT_T0(sp)
	sd x6,  PT_T1(sp)
	sd x7,  PT_T2(sp)
	sd x8,  PT_S0(sp)
	sd x9,  PT_S1(sp)
	sd x10, PT_A0(sp)
	sd x11, PT_A1(sp)
	sd x12, PT_A2(sp)
	sd x13, PT_A3(sp)
	sd x14, PT_A4(sp)
	sd x15, PT_A5(sp)
	sd x16, PT_A6(sp)
	sd x17, PT_A7(sp)
	sd x18, PT_S2(sp)
	sd x19, PT_S3(sp)
	sd x20, PT_S4(sp)
	sd x21, PT_S5(sp)
	sd x22, PT_S6(sp)
	sd x23, PT_S7(sp)
	sd x24, PT_S8(sp)
	sd x25, PT_S9(sp)
	sd x26, PT_S10(sp)
	sd x27, PT_S11(sp)
	sd x28, PT_T3(sp)
	sd x29, PT_T4(sp)
	sd x30, PT_T5(sp)
	sd x31, PT_T6(sp)

	ld s0, USER_SP_OFFSET(tp)
	sd s0, PT_SP(sp)

	csrr s1, sstatus
	sd s1, PT_SSTATUS(sp)

	csrr s2, sepc
	sd s2, PT_SEPC(sp)
	
	csrr s3, sbadaddr
	sd s3, PT_SBADADDR(sp)

	csrr s4, scause
	sd s4, PT_SCAUSE(sp)

	csrr s5, sscratch
	sd s5, PT_TP(sp)

.endm

.macro kernel_exit
	ld a0, PT_SSTATUS(sp)
	csrw sstatus, a0

	ld a2, PT_SEPC(sp)
	csrw sepc, a2

	ld x1,  PT_RA(sp)
	ld x3,  PT_GP(sp)
	ld x4,  PT_TP(sp)
	ld x5,  PT_T0(sp)
	ld x6,  PT_T1(sp)
	ld x7,  PT_T2(sp)
	ld x8,  PT_S0(sp)
	ld x9,  PT_S1(sp)
	ld x10, PT_A0(sp)
	ld x11, PT_A1(sp)
	ld x12, PT_A2(sp)
	ld x13, PT_A3(sp)
	ld x14, PT_A4(sp)
	ld x15, PT_A5(sp)
	ld x16, PT_A6(sp)
	ld x17, PT_A7(sp)
	ld x18, PT_S2(sp)
	ld x19, PT_S3(sp)
	ld x20, PT_S4(sp)
	ld x21, PT_S5(sp)
	ld x22, PT_S6(sp)
	ld x23, PT_S7(sp)
	ld x24, PT_S8(sp)
	ld x25, PT_S9(sp)
	ld x26, PT_S10(sp)
	ld x27, PT_S11(sp)
	ld x28, PT_T3(sp)
	ld x29, PT_T4(sp)
	ld x30, PT_T5(sp)
	ld x31, PT_T6(sp)

	ld x2,  PT_SP(sp)
.endm

.align 2
.global cpu_switch_to
cpu_switch_to:
	li    a4,  CONTEXT_OFFSET
	add   a3, a0, a4
	add   a4, a1, a4

	//save prev process context
	sd ra, 0(a3)
	sd sp, 8(a3)
	sd s0, 16(a3)
	sd s1, 24(a3)
	sd s2, 32(a3)
	sd s3, 40(a3)
	sd s4, 48(a3)
	sd s5, 56(a3)
	sd s6, 64(a3)
	sd s7, 72(a3)
	sd s8, 80(a3)
	sd s9, 88(a3)
	sd s10, 96(a3)
	sd s11, 104(a3)

	//load next process context
	ld ra, 0(a4)
	ld sp, 8(a4)
	ld s0, 16(a4)
	ld s1, 24(a4)
	ld s2, 32(a4)
	ld s3, 40(a4)
	ld s4, 48(a4)
	ld s5, 56(a4)
	ld s6, 64(a4)
	ld s7, 72(a4)
	ld s8, 80(a4)
	ld s9, 88(a4)
	ld s10, 96(a4)
	ld s11, 104(a4)

	move tp, a1
	ret

.align 2
.global ret_from_kernel_thread
ret_from_kernel_thread:
	call schedule_tail
	//jump to callback function
	la ra, ret_from_exception
	move a0, s1
	jr s0

.align 2
.global do_exception_vector
do_exception_vector:
    kernel_entry
	csrw sscratch, x0
    la ra, ret_from_exception
    mv a0, sp
    mv a1, s4	//scause
    tail do_exception

ret_from_exception:
	ld s0, PT_SSTATUS(sp)
	csrc sstatus, SSTATUS_SIE
	andi s0, s0, SSTATUS_SPP		/*judge whether is S mode interrupt*/
	bnez s0, ret_to_kernel

ret_to_user:
	lw s0, NEED_SCHEDULE_OFFSET(tp)
	andi s0, s0, 1
	bnez s0, need_schedule

no_need_schedule:
	addi s0, sp, PT_SIZE
	sd s0, KERNEL_SP_OFFSET(tp)
	csrw sscratch, tp
	j restore_all

need_schedule:
	call schedule
	j no_need_schedule

ret_to_kernel:
	/*judge whether can preempt or not*/
	lw s0, SCRAMBLE_OFFSET(tp)
	beqz s0, restore_all

can_preempt:
	/*whether need to preempt*/
	lw s0, NEED_SCHEDULE_OFFSET(tp)
	andi s0, s0, 1
	beqz s0, restore_all
	/*preempt*/
	call preempt_schedule
	j can_preempt
	
restore_all:
    kernel_exit
    sret

.global ret_from_fork
ret_from_fork:
	/*back to U mode*/
	la ra, ret_from_exception
	tail schedule_tail

.global enable_mmu
enable_mmu:
la a3, _pgd_page_begin
srl a3, a3, 12
lui a1, 1 
slli a1, a1, 63
or a3, a3, a1
sfence.vma
csrw satp, a3
ret
