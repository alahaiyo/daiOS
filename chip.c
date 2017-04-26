  
#define readl(_a)        (*(volatile unsigned int *)(_a))
#define writel(_v, _a)   (*(volatile unsigned int *)(_a) = (_v))

#define HI_CHIP_ID (0x003516c300LL)
/******************************************************************************/

static inline void udelay(unsigned int cnt)
{
	while (cnt--)
		__asm__ __volatile__("nop");
}

/******************************************************************************/

long long get_chipid_reg(void)
{
	long long chipid = 0;

	chipid = HI_CHIP_ID;

	return chipid;
}

/******************************************************************************/

void reset_cpu(unsigned long addr)
{
	while (1)
		/* Any value to this reg will reset the cpu */
		writel(0x12345678, (0x12020000 + 0x4));
}
/******************************************************************************/

int do_reset()
{
	printk ("resetting ...\n");
	udelay (50000);				/* wait 50 ms */
	disable_interrupts ();
	reset_cpu(0);

	/*NOTREACHED*/
	return 0;
}
