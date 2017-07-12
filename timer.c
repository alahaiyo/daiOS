

#define TIMER0_LOAD        (*(volatile unsigned int *)0x12000000)
#define TIMER0_CONTROL     (*(volatile unsigned int *)0x12000008)
#define TIMER0_INTCLR      (*(volatile unsigned int *)0x1200000c)
#define SC_CTRL            (*(volatile unsigned int *)0x12020000)
#define TIMER0_RIS         (*(volatile unsigned int *)0x12020010)

#define INT_INTENABLE_ADDR      (*(volatile unsigned int *)0x10040010)
#define INT_INTENCLEAR_ADDR     (*(volatile unsigned int *)0x10040014)

void enable_timer0()
{
	TIMER0_CONTROL |= (1 << 7);
}

void disable_timer0()
{
	TIMER0_CONTROL &= (~(1 << 7));
}

void timer_init()
{
	TIMER0_LOAD = 0x26e50;      //1s	
	/* 设置时钟频率  50MHZ */
	SC_CTRL |= (1 << 16);
	
	TIMER0_CONTROL &= (~(1 << 0));  //循环模式
	TIMER0_CONTROL |= (1 << 1);     //32bit计数器
	TIMER0_CONTROL |= (1 << 3);;    //8分频
	TIMER0_CONTROL |= (1 << 5);     //不屏蔽中断
	TIMER0_CONTROL |= (1 << 6);     //周期模式
	
	INT_INTENABLE_ADDR |= (1 << 3);
	INT_INTENCLEAR_ADDR &= (~(1 << 3));
	
	TIMER0_INTCLR = 0xff;
	enable_timer0();
//	enable_interrupts();
	
	
}

void timer_irq_handle()
{
	unsigned int a = TIMER0_RIS;
	TIMER0_INTCLR = 0xff;
	//disable_interrupts();
	printk("Timer interrupt! TIMER0_RIS = %x\r\n", a);
	
}


