
#define INT_INTSTATUS     (*(volatile unsigned int *)0x10040000)
#define INT_RAMINTR     (*(volatile unsigned int *)0x10040008)

void do_irq()
{
	unsigned int a = INT_INTSTATUS;
	unsigned int b = INT_RAMINTR;
	
	printk("do irq INT_INTSTATUS = %x \r\n", a);
	printk("do irq INT_RAMINTR = %x \r\n", b);
	
	if(INT_INTSTATUS & (1 << 3)) {
		timer_irq_handle();
	}
	
	if(INT_INTSTATUS & (1 << 31)) {
		gpio_irq_handle();
	}
}