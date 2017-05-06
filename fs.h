#define MAX_SUPER_BLOCK	(8)

#define DAIFS	0

struct super_block;


struct inode{
	char *name;
	unsigned int flags;
	unsigned int dsize;			    //file data size
	unsigned int daddr;				//real data bias inside the block device
	struct super_block *super;
	
};

struct super_block{
	struct inode *(*namei)(struct super_block *super,char *p);
	unsigned int (*get_daddr)(struct inode *);
	
	struct storage_device *device;
	char *name;
};

extern struct super_block *fs_type[];
