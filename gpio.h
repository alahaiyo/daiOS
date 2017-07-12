#ifndef __GPIO_H
#define __GPIO_H

#define GPIO7_DIR_ADDR          (*(volatile unsigned int *)0x12147400)
#define GPIO7_BASE_ADDR         (*(volatile unsigned int *)0x12147000)
#define GPIO7_IS_ADDR           (*(volatile unsigned int *)0x12147404)
#define GPIO7_IEV_ADDR          (*(volatile unsigned int *)0x1214740c)
#define GPIO7_MIS_ADDR          (*(volatile unsigned int *)0x12147418)
#define GPIO7_IC_ADDR           (*(volatile unsigned int *)0x1214741c)
#define GPIO7_IE_ADDR           (*(volatile unsigned int *)0x12147410)

extern unsigned int gpio_base[9];


#define DATA_REG(bank, pin) (gpio_base[bank] + (1 << (pin + 2)))
#define DIR_REG(bank)       (gpio_base[bank] + 0x400)

#define gpio_dirin(b, p)  do {*(volatile unsigned int *)(DIR_REG(b)) &= ~(1 << (p));} while (0)
#define gpio_dirout(b, p) do {*(volatile unsigned int*)(DIR_REG(b)) |= 1 << (p);} while (0)
#define gpio_set(b, p)    do {*(volatile unsigned int*)(DATA_REG(b, p)) = 1 << (p);} while (0)
#define gpio_clr(b, p)    do {*(volatile unsigned int*)(DATA_REG(b, p)) = 0 << (p);} while (0)
#define gpio_get(b, p)    ((*(volatile unsigned int*)(DATA_REG(b, p)) & (1 << (p)))? 1 : 0)
#define gpio_dir(b, p)    ((*(volatile unsigned int*)(DIR_REG(b)) & (1 << (p)))? 1 : 0)

#define GPIO_IN		(0)
#define GPIO_OUT	(1)
#define GPIO_HIGH	(1)
#define GPIO_LOW	(0)


#endif