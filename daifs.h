#ifndef __DAIFS_H_
#define __DAIFS_H_

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

#endif