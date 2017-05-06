/*
daifs.c
Copyright (C) 2017/5  Kunhai Dai <daikunhai@163.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/.
*/

#include "fs.h"
#include "storage.h"

#define MAX_INODE_NUMBER 128-1
#define MAX_INODE_PAGE   4
#define INODE_SIZE       64
#define SUPER_BLOCK_SIZE 64
#define PAGE_SIZE        2048
#define DAIFS_MAX_FILE_NAME  44

#define NORMAL_FILE     1
#define DIRECTORY_FILE  2

struct daifs_super_block {
	unsigned int s_word0;
	unsigned int s_word1;
	unsigned int s_size;
	unsigned int s_checksum;
	char s_name[48];
};

struct daifs_inode {
	unsigned int i_mode;
	unsigned int i_offset;
	unsigned int i_length;
	unsigned int i_num;
	unsigned int i_father_num;	
	char i_name[44];
};

struct daifs_inode *daifs_inode_table;

static char *bmap(char *tmp,char *dir)
{
	unsigned int n;
	char *p=strchr(dir,'/');
	if(!p){
		strcpy(tmp,dir);
		return NULL;
	}
	n=p-dir;
	//n=(n>ROMFS_MAX_FILE_NAME)?ROMFS_MAX_FILE_NAME:n;
	strncpy(tmp,dir,n);
	return p+1;
}

static char *get_the_file_name(char *p,char *name)
{
	char *tmp=p;
	int index;
	for(index=0;*tmp;tmp++){
		if(*tmp=='/'){
			index=0;
			continue;
		}else{
			name[index]=*tmp;
			index++;
		}
	}
	name[index]='\0';
	return name;
}

struct inode *daifs_namei(struct super_block *block, char *dir)
{
	int i;
	struct inode *get_inode;
	
	char name[DAIFS_MAX_FILE_NAME],fname[DAIFS_MAX_FILE_NAME];
	
	get_the_file_name(dir,fname);
//	printk("fname = %s\r\n", fname);
	dir = bmap(name,dir);
//	printk("name = %s\r\n", name);
	for(i = 0; i < MAX_INODE_NUMBER; i++) {
//		printk("inode name = %s\r\n", daifs_inode_table[i].i_name);
		if(!strcmp(name, daifs_inode_table[i].i_name)) {
			if(!strcmp(name, fname))
				goto FOUND;
			else {
				dir = bmap(name, dir);
				if(dir == NULL) {
					goto FOUNDDIR;
				}
			}
		} 
	}
	printk("File not found!\r\n");
	return NULL;
FOUNDDIR:
FOUND:
	if((get_inode = (struct inode *)kmalloc(sizeof(struct inode), 0))==NULL){
		goto KMALLOC_FAILED;
	}
	get_inode->name = daifs_inode_table[i].i_name;
	get_inode->dsize = daifs_inode_table[i].i_length;
	get_inode->daddr = daifs_inode_table[i].i_offset * block->device->sector_size;
	get_inode->super = block;
	
	return get_inode;
	
KMALLOC_FAILED:	
	return NULL;
}


struct super_block daifs_sb={
	.namei = daifs_namei,
	.name = "daifs",
};

int daifs_init(void)
{
	int ret = 0;
	char *head_data;
	struct daifs_super_block *dai_sb;
	
	head_data = (char *)get_free_pages(2);
	if(head_data == NULL) {
		printk("head_data: Kmalloc failed!\r\n");
		return -1;
	}
	memset(head_data, 0, 2048 * 4);
	
	dai_sb = (struct daifs_super_block *)head_data;
	daifs_inode_table = (struct daifs_inode*)(head_data + SUPER_BLOCK_SIZE);
	
	ret = register_file_system(&daifs_sb,DAIFS);
	daifs_sb.device = storage[NAND_FLASH];
	
	daifs_sb.device->read_data(daifs_sb.device, head_data, 0x6400000, 2048 * 4);

	printk("filesystem name : daifs\r\n");
	printk("         Author : %s\r\n", dai_sb->s_name);
	
	return ret;
}









