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
	 * set the cpu to SYS mode
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
.equ DISABLE_IRQ,0x80
.equ DISABLE_FIQ,0x40
.equ SYS_MOD,0x1f
.equ IRQ_MOD,0x12
.equ FIQ_MOD,0x11
.equ SVC_MOD,0x13
.equ ABT_MOD,0x17
.equ UND_MOD,0x1b
.equ MOD_MASK,0x1f


.macro CHANGE_TO_SVC
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SVC_MOD)
.endm

.macro CHANGE_TO_IRQ
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|IRQ_MOD)
.endm

.macro CHANGE_TO_SYS
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SYS_MOD)
.endm
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
	stmfd r13!,{r0}	
	stmfd r13!,{r1-r3} //r0-r3压入中断模式的堆栈中	
/*	
	ldr r2, =0x10040000 //INT_IRQSTATUS
	ldr r1, [r2]
	tst r1, #0x8   //查询中断状态寄存器，Timer0/Timer1
	bne timer_irq   //=0跳转

other_irq:	
	stmfd r13!,{r14}	
	bl do_irq
	ldmfd r13!, {r4}
	ldmfd r13!, {r1-r3}
	ldmfd r13!, {r0}
	stmfd r13!, {r4}
	ldmfd r13!, {pc}^  //r14恢复到pc，直接跳转回去

	
timer_irq:	
	ldr r2, =0x12000010 //查询是否发生定时器中断
	ldr r1, [r2]
	tst r1, #0x1
*/
	
	ldr r2, =0x1200000c
	mov r1, #0xff
	str r1, [r2]  //清中断

	mov r1,r14          //r14给到r1，这里的r14存放着中断之前的执行地址，也就是中断返回后要给PC去继续执行的
	
	
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr,r0      //切回到系统模式 ---------------------------------------------------------------
	
	stmfd r13!,{r1}    //先把r0压栈，也就是中断时刻要本来将要执行的地址                     sys   |
	stmfd r13!,{r14}   //然后r14压栈，这个保持上个函数返回地址                                    |
/*
	mrs	r0, cpsr
	bic	r0, r0, #0x12
	orr	r0, r0, #0xd3
	msr	cpsr,r0  */
    CHANGE_TO_IRQ    //回到中断模式------------------------------------------------------------------
	
	ldmfd r13!,{r1-r3}  //把r1-r3出栈
	ldmfd r13!,{r0}    //把之前入栈的r0弹出来，这个时候，我们已经回到刚进中断时候的状态了。
	ldr r14,=__asm_schedule       //                                                        irq
	stmfd r13!,{r14}   // 先压栈，再出栈，跳转到__asm_schedule。
	ldmfd r13!,{pc}^  // 然后返回到用户模式------------------------------------------------------------

__asm_schedule:
	stmfd r13!,{r0-r12}   //用户模式下，保护上一个进程现场
	mrs	r1, cpsr
	stmfd r13!,{r1}

	mov	r1,sp
	bic	r1,#0xff0
	bic r1,#0xf   //此时r1的值就是进程内存的低地址
	mov r0,sp    
	str r0,[r1]   //保存上一个进程的sp指针到进程最低地址，ok，保存完毕！
/*	
	ldr r2, =0x12141080
	mov r1, #0x20
	str r1, [r2]  //蓝灯
*/	
	bl __common_schedule
	
	ldr sp,[r0]     //返回的地址给SP，下一个进程的sp
	ldmfd r13!,{r1} 
	msr cpsr_cxsf,r1
	ldmfd r13!,{r0-r12,r14,pc}	//恢复现场，pc指针执行进程下一条指令。
	
	.align	5
fiq:
	nop
	
halt:
	b halt

	


	
	
	
	
	
	
	
	

