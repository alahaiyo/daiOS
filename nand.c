
#include "storage.h"

#define TIMING_SPI_CFG     0x10000008     
#define FMC_CFG            0x10000000  //0x65
#define GLOBAL_CFG         0x10000004
#define FMC_CMD            0x10000024
#define FMC_OP             0x1000003c
#define FMC_ADDRL          0x1000002c
#define FMC_OP_CFG         0x10000030
#define FMC_FLASH_INFO     0x100000ac
#define FMC_DMA_SADDR_D0   0x1000004c
#define FMC_DMA_LEN        0x10000040
#define FMC_OP_CTRL        0x10000068
#define FMC_INT_CLR        0x10000020
 
#define readl(_a)        (*(volatile unsigned int *)(_a))
#define writel(_v, _a)   (*(volatile unsigned int *)(_a) = (_v))


#define TIMING_CFG_TCSH(_n)			(((_n) & 0xf) << 8)
#define TIMING_CFG_TCSS(_n)			(((_n) & 0xf) << 4)
#define TIMING_CFG_TSHSL(_n)			((_n) & 0xf)

#define CS_HOLD_TIME				0x6
#define CS_SETUP_TIME				0x6
#define CS_DESELECT_TIME			0xf


#define NAND_PAGE_SIZE 2048
#define NAND_CHIP_SIZE 256*1024*1024
#define NAND_BLOCK_SIZE 128*1024


void nand_init()
{
	unsigned int reg;
	
	reg = TIMING_CFG_TCSH(CS_HOLD_TIME)
		| TIMING_CFG_TCSS(CS_SETUP_TIME)
		| TIMING_CFG_TSHSL(CS_DESELECT_TIME);
	
	writel(reg, TIMING_SPI_CFG);
	
	reg = readl(FMC_CFG);
	reg |= 0x1;
	writel(reg, FMC_CFG);
	
	reg = readl(GLOBAL_CFG);
	reg &= ~(1 << 6);
	writel(reg, GLOBAL_CFG);
	
	
}

void udelay(volatile unsigned int i)
{
	i *= 10;
	while(i--);
}

void wait_cpu_finish()
{
	unsigned int timeout = 400000;
	unsigned int reg;
	
	do {
		udelay(10);
		reg =  readl(FMC_OP);
		--timeout;
	} while((reg & 0x01) && timeout);
	if(!timeout)
		printk("Error: Wait cmd cpu finish timeout!\r\n");
}

int get_status()
{
	unsigned int reg;
	
	writel(0x00, FMC_OP_CFG);
	writel(0x03, FMC_OP);
	wait_cpu_finish();
	reg = readl(FMC_FLASH_INFO);
	if(reg & 0x01)
		return 1;
	else
		return 0;
}

int nand_erase_block(unsigned int addr)
{
	unsigned int block;
	if(addr & 0x1ffff){
		printk("Nand erase addr is not aligned!\r\n");
		return -1;
	}
	block = addr >> 11;
	
	while(get_status());

	/*write enable*/
	while(get_status());
	writel(0x84, GLOBAL_CFG);
	writel(0x06, FMC_CMD);
	writel(0, FMC_OP_CFG);
	writel(0x81, FMC_OP);
	wait_cpu_finish();

	while(get_status());
	while(get_status());

	writel(0xff, FMC_INT_CLR);
	writel(0xd8, FMC_CMD);
	writel(block, FMC_ADDRL);
	writel(0x30, FMC_OP_CFG);
	writel(0xc1, FMC_OP);
	wait_cpu_finish();
	while(get_status());
	
	return 0;
}
 
 
int nand_read_page(unsigned int addr, char *buf)
{
	unsigned int reg;
	unsigned int page;
	
	if(addr & 0x7ff)
		return -1;
	page = addr << 5;
	
	if(buf == NULL)
		return -1;
	
	while(get_status());
	writel(0xff, FMC_INT_CLR);
	
	reg = readl(FMC_CFG);
	reg |= 0x01;
	writel(reg, FMC_CFG);
	writel(page, FMC_ADDRL);
	writel(buf, FMC_DMA_SADDR_D0);
	writel(2048, FMC_DMA_LEN);
	writel(0x201, FMC_OP_CFG);
	writel(0xeb0001, FMC_OP_CTRL);
	
	while((readl(FMC_OP_CTRL) & 0x01) == 1);
	while(get_status());
	
	return 0;
}

int nand_write_page(unsigned int addr, char *buf)
{
	unsigned int reg;
	unsigned int page;
	
	if(addr & 0x7ff)
		return -1;
	page = addr << 5;
	
	if(buf == NULL)
		return -1;
	
	while(get_status());
	
	/*write enable*/
	while(get_status());
	writel(0x84, GLOBAL_CFG);
	writel(0x06, FMC_CMD);
	writel(0, FMC_OP_CFG);
	writel(0x81, FMC_OP);
	wait_cpu_finish();
	
	writel(0xff, FMC_INT_CLR);
	reg = readl(FMC_CFG);
	reg |= 0x01;
	writel(reg, FMC_CFG);
	writel(page, FMC_ADDRL);
	writel(buf, FMC_DMA_SADDR_D0);
	writel(2048, FMC_DMA_LEN);
	writel(0x180, FMC_OP_CFG);
	writel(0x3203, FMC_OP_CTRL);
	
	while((readl(FMC_OP_CTRL) & 0x01) == 1);
	while(get_status());
	
	return 0;
}

/*
 *  Nand Flash 读数据函数
 * 
 *  *sd 存储设备结构体， *dest 缓存， start 起始地址， size 读写数据大小 
*/
int nand_read(struct storage_device *sd, void *dest, unsigned int start, size_t size)
{
	int ret;
	unsigned int addr;
	int pagesize = sd->sector_size;
	char *buf = (char *)dest;
	
	if(start % pagesize!=0) {
		printk("nand_read: Start addr is not alignment!\r\n");
		return -1;
	}
	
	if(size % pagesize!=0) {
		printk("nand_read: Read size is not alignment!\r\n");
		return -1;
	}
	
	if(sd->start_pos + size > sd->storage_size) {
		printk("nand_read: Read size is out of rang!\r\n");
		return -1;
	}
	//NAND_ENABLE_CE();
	addr = sd->start_pos + start;
	
	printk("nand read: addr = %x; size = %d\r\n", addr, size);
	while(size > 0) {
		
		nand_read_page(addr, buf);
		buf += pagesize;
		size -= pagesize;
		addr += pagesize;
		
	}
	
	return 0;
	
}


struct storage_device nand_stroage_device = {
	//.write_data = nand_write,
	.read_data = nand_read,
	.sector_size = NAND_PAGE_SIZE,
	.storage_size = NAND_CHIP_SIZE,
	.start_pos = 0,
};

int nand_device_init()
{
	int ret;
	
	nand_init();
	ret = register_storage_device(&nand_stroage_device, NAND_FLASH);
	if(!ret)
		return 0;
	else
		return -1;
}


