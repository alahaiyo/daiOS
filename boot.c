#include "gpio.h"
#include "libc.h"
#include "storage.h"
#include "fs.h"
#include "mem.h"

#define	NULL	((void *)0)

extern int do_reboot();

struct task_info{
	unsigned int sp;
	struct task_info *next;
};

void delay()
{
	volatile  int i = 0x300000;
	while(i--);
}

int  test_process(void *p)
{
	while(1){
		delay();
		printk("This is process %dth!!!!\r\n",(int)p);
	}
	return 0;
}


void start_armboot(void)
{
	char *buf = NULL;
	struct page *my_page;
	int i = 0;
	struct inode *test_inode;
	struct task_info *idle_process;
	register unsigned long sp asm ("sp");
	unsigned int sp_tmp;

	printk("System start...\r\n");
	GPIO7_DIR_ADDR &= (~(1 << 5));
//	GPIO7_DIR_ADDR = 0x13;
	//printk("GPIO7_DIR_ADDR = %x\r\n", GPIO7_DIR_ADDR);
	led_init();
	//irq_init();
	
	kmalloc_init();
	
	nand_device_init();
	
	daifs_init();
//----------------------------------------------------	
	buf = (char *)kmalloc(2*1024);
	if(buf == NULL)
		printk("1buf: kmalloc failed!\r\n");
	printk("1buf addr = %x\r\n", buf);

//---------------------------------------------------	
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
	//=======================================

	printk("\r\nNow, we start creat a process!\r\n");

	task_init();
//	i=do_fork(test_process,(void *)0x1);
//	i=do_fork(test_process,(void *)0x2);
	
	timer_init();
//	enable_interrupts ();
	
	
end:	
//	printk("\r\nPress button to reboot!\r\n");
	
	shell_main_loop();
	while(1) {
		if(!gpio_get(7, 5)) {
		//	printk("Low\r\n");	
			do_reset();
		}
		
		printk("\r\nThis is dile process.\r\n");
		delay();
	}
}
