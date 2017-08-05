
int do_reboot()
{
	printk ("resetting ...\n");
	udelay (50000);				/* wait 50 ms */
	disable_interrupts ();
	reset_cpu(0);

	/*NOTREACHED*/
	return 0;	
}

