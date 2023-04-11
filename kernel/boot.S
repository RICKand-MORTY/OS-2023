.section ".text.boot"

.globl _start
_start:
	/* Mask all interrupts */
	csrw sie, zero

	/*size 4KB for stack*/
	la sp, stacks_start
	li t0, 4096
	add sp, sp, t0

	tail kernel_main

.section .data
.align  12
.global stacks_start
stacks_start:
	.skip 4096

