#include <scheduler.h>
.section ".text.boot"

.globl _start
_start:
	/* Mask all interrupts */
	csrw sie, zero

	/*create stack for init process*/
	la sp, init_task_union
	li t0, THREAD_SIZE
	add sp, sp, t0
	la tp, init_task_union

	tail kernel_main

.section .data
.align  12
.global stacks_start
stacks_start:
	.skip 4096
