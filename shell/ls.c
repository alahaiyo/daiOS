#include "../daifs.h"

extern unsigned int current_dir_inode;
extern struct daifs_inode *daifs_inode_table;

void do_ls()
{
	int i;
	
	for(i = 0; i < MAX_INODE_NUMBER; i++) {
		if( daifs_inode_table[i].i_father_num == current_dir_inode ) {
			printk("%d: %s\r\n", i, daifs_inode_table[i].i_name);
		} 
	}
}