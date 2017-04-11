#include "gpio.h"

void delay()
{
	volatile  int i = 0x500000;
	while(i--);
}
void start_armboot(void)
{
	printk("System start...\r\n");
	GPIO7_DIR_ADDR &= (~(1 << 5));
	irq_init();
	
	while(1) {
		if(!gpio_get(7, 5)) {
			printk("Low\r\n");
			delay();
		}
		
	}
}
