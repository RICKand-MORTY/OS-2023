#include "syscall_num.h"

.global __clone
__clone:
    addi a1, a1, -16
	sd a0, (a1)
	sd a3, 8(a1)    /*save callback function and argument*/

	/* call syscall*/
	move a0, a2
	li     a7, SYS_clone
	ecall

	beqz a0, thread_start
	ret

.align 2
thread_start:
	ld a1, (sp)     //function
	ld a0, 8(sp)    //argument

	/* 调用clone的回调函数fn() */
	jalr   a1

	/* exit */
	ret
