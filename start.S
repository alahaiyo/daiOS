/*
*  start.S
*  启动代码
*/

#include "platform.h"

.globl _start
_start: b	reset
	ldr	pc, _undefined_instruction  
	ldr	pc, _software_interrupt    
	ldr	pc, _prefetch_abort	 
	ldr	pc, _data_abort	
	ldr	pc, _not_used
	ldr	pc, _irq		
	ldr	pc, _fiq		  

_undefined_instruction: .word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq


.globl _bss_start
_bss_start:
	.word __bss_start

.globl _bss_end
_bss_end:
	.word __bss_end

/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de
	
/*
 * the actual reset code
 */

reset:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr,r0

	/*
	 * we do sys-critical inits only at reboot,
	 * not when booting from ram!
	 */

	/*
	 * flush v4 I/D caches
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	/* clear bits 13, 9:8 (--V- --RS) */
	bic	r0, r0, #0x00000087	/* clear bits 7, 2:0 (B--- -CAM) */
	orr	r0, r0, #0x00000002	/* set bit 2 (A) Align */
	mcr	p15, 0, r0, c1, c0, 0


normal_start_flow:
	/* init serial and printf a string. */
	ldr	sp, =0x04018000     //ram in the chip
	bl      uart_early_init
	bl      msg_main_cpu_startup

	
	
do_clr_remap:
    	/* do clear remap */
    	ldr     r4, =SYS_CTRL_REG_BASE
	ldr 	r0, [r4, #REG_SC_CTRL]
	/* reg[8]:
	 * 0:	keep remap
	 * 1:	clear remap
	 */
	@Set clear remap bit.
	orr 	r0, #REMAPCLEAR
	str 	r0, [r4, #REG_SC_CTRL]

	@enable I-Cache now
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x00001000 /* set bit 12 (I) I-Cache */
	mcr	p15, 0, r0, c1, c0, 0
	
	/* Set up the stack */
stack_setup:
	ldr sp, =0x8ffffffc

	/* Clear BSS (if any). Is below tx (watch load addr - need space) */
clear_bss:
	ldr	r0, _bss_start		@ find start of bss segment
	ldr	r1, _bss_end		@ stop here
	mov	r2, #0x0		@ clear value
clbss_l:
	str	r2, [r0]		@ clear BSS location
	cmp	r0, r1			@ are we at the end yet
	add	r0, r0, #4		@ increment clear index pointer
	bne	clbss_l			@ keep clearing till at end

	ldr	pc, _start_armboot	@ jump to C code
	
loop:
	b loop
	
_start_armboot: .word start_armboot




@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
.align  2
msg_main_cpu_startup:
	mov     r5, lr
	add     r0, pc, #4
	bl      uart_early_puts
	mov     pc, r5
L10:
	.ascii "\r\n\r\n##### dai OS #####\r\n\r\n\0"


@******************************************************************************
@
@  void uart_early_init(void);
@
.text
.align	2
.global	uart_early_init
.type	uart_early_init, %function
uart_early_init:
	ldr	a4, uart_base_addr_L0
	mov	a3, #0
	/* Disable UART */
	str	a3, [a4, #48]
	/* Set baud rate to 115200, uart clock:24M */
	add	a3, a3, #13
	str	a3, [a4, #36]
	mov	a3, #1
	str	a3, [a4, #40]
	/* Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled. */
	ldr	a3, =112
	str	a3, [a4, #44]
	/* Enable UART */
	ldr	a3, =769
	str	a3, [a4, #48]
	bx	lr
uart_base_addr_L0:
	.word 0x12100000

@******************************************************************************
@
@  void uart_early_puts(const char *ss);
@
.align	2
.global	uart_early_puts
.type	uart_early_puts, %function
uart_early_puts:
	ldr	a2, uart_base_addr_L1
	b	next_char
output:
	ldr	a4, [a2, #24]//查看寄存器第6bit，
	tst	a4, #32
	bne	output
	str	a3, [a2, #0]//要发生的字符传送到UART_DR
	add	a1, a1, #1
next_char:
	ldrb	a3, [a1]//a1中取一个字符
	cmp	a3, #0
	bne	output
	bx	lr
uart_base_addr_L1:
	.word 0x12100000

	
/*
*   Interrupt handling
*/

/*
 * exception handlers
 */
	.align	5
undefined_instruction:
	nop

	.align	5
software_interrupt:
	nop

	.align	5
prefetch_abort:
	nop

	.align	5
data_abort:
	nop

	.align	5
not_used:
	nop

	.align	5
irq:
	ldr sp, =0x8f600000
	sub r14, r14, #4
	stmfd r13!, {r0-r3, r14}
	bl do_irq
	ldmfd r13!, {r0-r3, pc}^

	.align	5
fiq:
	nop
	
halt:
	b halt

	


	
	
	
	
	
	
	
	

