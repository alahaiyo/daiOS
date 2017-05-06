#ifndef __STORAGE_H__
#define __STORAGE_H__

#define	NULL	((void *)0)

#define MAX_STORAGE_DEVICE (2)
#define RAMDISK	0
#define NAND_FLASH 1

typedef unsigned int size_t;

struct storage_device{
	unsigned int start_pos;
	size_t sector_size;
	size_t storage_size;
	int (*write_data)(struct storage_device *sd, void *dest, unsigned int start, size_t size);
	int (*read_data)(struct storage_device *sd, void *dest, unsigned int  start, size_t size);
};

extern struct storage_device *storage[MAX_STORAGE_DEVICE];
extern int register_storage_device(struct storage_device *sd,unsigned int num);

#endif
