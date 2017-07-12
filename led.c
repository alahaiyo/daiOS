#include "gpio.h"
void led_init()
{
	gpio_dirout(1, 5);
	gpio_dirout(1, 0);
	
	gpio_clr(1, 5);
	gpio_clr(1, 0);
}

void led1_red()
{
	gpio_clr(1, 5);
}

void led2_red()
{
	gpio_clr(1, 0);
}

void led1_blue()
{
	gpio_set(1, 5);
}

void led2_blue()
{
	gpio_set(1, 0);
}