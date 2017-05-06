#include "gpio.h"
#include "libc.h"
#include "storage.h"
#include "fs.h"

#define	NULL	((void *)0)

void delay()
{
	volatile  int i = 0x500000;
	while(i--);
}
void start_armboot(void)
{
	char *buf = NULL;
	int i = 0;
	struct inode *test_inode;
	
	printk("System start...\r\n");
	GPIO7_DIR_ADDR &= (~(1 << 5));
	//irq_init();
	//timer_init();
	kmalloc_init();
	//nand_init();
	nand_device_init();
	
	daifs_init();
	
	buf = (char *)kmalloc(2*1024);
	if(buf == NULL)
		printk("buf: kmalloc failed!\r\n");
	printk("buf addr = %x\r\n", buf);
	
	//nand_read_page(0x6400000, buf);
	//storage[NAND_FLASH]->read_data(storage[NAND_FLASH], buf, 0x6400000, 2048);
	

	test_inode = fs_type[DAIFS]->namei(fs_type[DAIFS], "a.txt");
	if(test_inode == NULL) {
		goto end;
	}
	printk("file addr = %d\r\n", test_inode->daddr);
	printk("file name : %s\r\n", test_inode->name);	
	fs_type[DAIFS]->device->read_data(fs_type[DAIFS]->device, buf, test_inode->daddr + 0x6400000, 2048);
	
	for(i = 0; i < 20; i++) {
		printk("%c ", buf[i]);
	}
	printk("\r\n");
	
end:	
	printk("\r\nPress button to reboot!\r\n");
	while(1) {
		if(!gpio_get(7, 5)) {
		//	printk("Low\r\n");
			delay();
			do_reset();
		}
		
	}
}
