#include "mem.h"

extern void *kmalloc(unsigned int size);

struct task_info{
	unsigned int sp;
	struct task_info *next;
};

#define TASK_SIZE	4096

struct task_info *current_task_info(void){
	register unsigned long sp asm ("sp");
	return (struct task_info *)(sp&~(TASK_SIZE-1));
}

#define current	current_task_info()

int task_init(void)
{
	current->next=current;
	return 0;
}
/*
struct task_info * task_init(void)
{
	struct task_info *tsk;
	struct page *task_page;
	
	task_page = alloc_pages(0, 1);//kmalloc(TASK_SIZE);
	tsk = (struct task_info *)task_page->vaddr;
	if(tsk == (void *)0) {
		printk("task info malloc failed!\r\n");
		return (void *)0;
	}
	printk("idle task addr = %x\r\n", (unsigned int)tsk);
	tsk->sp=((unsigned int)(tsk)+TASK_SIZE);
	
	tsk->next = tsk;
	
	printk("idle task sp = %x, next = %x\r\n", tsk->sp, (void *)tsk->next);

	return tsk;
}
*/
#define disable_schedule(x)	disable_interrupts()
#define enable_schedule(x)	enable_interrupts()

#define DO_INIT_SP(sp,fn,args,lr,cpsr,pt_base)	\
		do{\
				(sp)=(sp)-4;/*r15*/		\
				*(volatile unsigned int *)(sp)=(unsigned int)(fn);/*r15*/		\
				(sp)=(sp)-4;/*r14*/		\
				*(volatile unsigned int *)(sp)=(unsigned int)(lr);/*r14*/		\
				(sp)=(sp)-4*13;/*r12,r11,r10,r9,r8,r7,r6,r5,r4,r3,r2,r1,r0*/		\
				*(volatile unsigned int *)(sp)=(unsigned int)(args);\
				(sp)=(sp)-4;/*cpsr*/		\
				*(volatile unsigned int *)(sp)=(unsigned int)(cpsr);\
		}while(0)



unsigned int get_cpsr(void)
{
	unsigned int p;
	asm volatile(
		"mrs %0,cpsr\n"
		:"=r"(p)
		:
	);
	return p;
}

int do_fork(int (*f)(void *),void *args)
{
	struct task_info *tsk,*tmp;
	struct page *task_page;
	
	task_page = alloc_pages(0, 1);//kmalloc(TASK_SIZE);
	tsk = (struct task_info *)task_page->vaddr;
	if(tsk == (void *)0) {
		printk("alloc task_info failed\n");
		return -1;
	}
	
	tsk->sp = ((unsigned int)(tsk)+TASK_SIZE);
	
	          //sp   r15(pc),r0-r12,r14, cprs,?
	DO_INIT_SP(tsk->sp, f, args, 0, 0x1f & get_cpsr(), 0);

	disable_schedule();
	tmp = current->next;
	current->next = tsk;
	tsk->next = tmp;
	enable_schedule();
printk("fork tmp = %x, current = %x\r\n", (void *)tmp, current);
	return 0;
}
void * __common_schedule(void)
{
//	printk("__common_schedule = %x\r\n", (void *)current->next);
	return (void *)(current->next);
}
