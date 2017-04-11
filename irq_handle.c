//#include "gpio.h"
#define GPIO7_DIR_ADDR          (*(volatile unsigned int *)0x12147400)
#define GPIO7_BASE_ADDR         (*(volatile unsigned int *)0x12147000)
#define GPIO7_IS_ADDR           (*(volatile unsigned int *)0x12147404)
#define GPIO7_IEV_ADDR          (*(volatile unsigned int *)0x1214740c)
#define GPIO7_MIS_ADDR          (*(volatile unsigned int *)0x12147418)
#define GPIO7_IC_ADDR           (*(volatile unsigned int *)0x1214741c)
#define GPIO7_IE_ADDR           (*(volatile unsigned int *)0x12147410)

#define INT_INTENABLE_ADDR      (*(volatile unsigned int *)0x10040010)
#define INT_INTENCLEAR_ADDR     (*(volatile unsigned int *)0x10040014)


/* enable IRQ interrupts */
void enable_interrupts (void)
{
	unsigned long temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "bic %0, %0, #0x80\n"
			     "msr cpsr_c, %0"
			     : "=r" (temp)
			     :
			     : "memory");
}


/*
 * disable IRQ/FIQ interrupts
 * returns true if interrupts had been enabled before we disabled them
 */
int disable_interrupts (void)
{
	unsigned long old,temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "orr %1, %0, #0xc0\n"
			     "msr cpsr_c, %1"
			     : "=r" (old), "=r" (temp)
			     :
			     : "memory");
	return (old & 0x80) == 0;
}

//7-5  默认高电平
void irq_init()
{
	/*配置引脚为中断功能*/
	GPIO7_DIR_ADDR &= (~(1 << 5));
	/*设置触发方式,边沿触发，下降沿，单边沿*/
	GPIO7_IS_ADDR |= (1 << 5);
	/*禁止屏蔽中断*/
	GPIO7_MIS_ADDR |= (1 << 5);
	/*清中断*/
	GPIO7_IC_ADDR = 0xff;
	/*中断控制器中使能中断*/
	GPIO7_IE_ADDR |= (1 << 5);
	INT_INTENABLE_ADDR |= (1 << 31);
	INT_INTENCLEAR_ADDR &= (~(1 << 31));
	
	enable_interrupts();
}

void do_irq(void)
{
	INT_INTENABLE_ADDR &= (~(1 << 31));
	GPIO7_IC_ADDR &= (~(1 << 5));
	printk("We are in irq handle\n\r");
	//delay();
}
