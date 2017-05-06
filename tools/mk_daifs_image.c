/*
 *
 * Copyright (C) 2017 Kunhai Dai
 *
 * Created by Kunhai Dai <daikunhai@hikvision.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
* make daifs image
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define MAX_INODE_NUMBER 128-1
#define MAX_INODE_PAGE   4
#define INODE_SIZE       64
#define SUPER_BLOCK_SIZE 64
#define PAGE_SIZE        2048

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

int fd_img;    /*输出文件句柄*/
static int error;
static int savedErrno;

static void fatal(const char *fn)
{
	perror(fn);
	error |= 1;
	exit(error);
}

static int warn(const char *fn)
{
	savedErrno = errno;
	perror(fn);
	error |= 2;
	return error;
}

int write_page(char *buf, int page, int size)
{
	int ret;
	
	ret = pwrite(fd_img, buf, PAGE_SIZE, page * PAGE_SIZE);
	if(ret != PAGE_SIZE)
		return -1;
	else
		return 0;
}

void dump_inode(struct daifs_inode t_inode)
{
	printf("\nMode = %x\n", t_inode.i_mode);
	printf("Offset = %x\n", t_inode.i_offset);
	printf("I-number = %x\n", t_inode.i_num);
	printf("I-father_num = %x\n", t_inode.i_father_num);
	printf("Length = %x\n", t_inode.i_length);
	printf("name = %s\n\n", t_inode.i_name);
}

int write_inode(unsigned int _inode_seq, struct stat *stats, unsigned int page_offset, int father_inode_num, char *name)
{
	int ret;
	unsigned inode_offset;
	struct daifs_inode t_inode;
	
	memset(t_inode.i_name, 0, sizeof(t_inode.i_name));
	
	if(S_ISREG(stats->st_mode)) {
		t_inode.i_mode = NORMAL_FILE;
		t_inode.i_length = stats->st_size;
	}
	
	if(S_ISDIR(stats->st_mode)) {
		t_inode.i_mode = DIRECTORY_FILE;
		t_inode.i_length = 0;
	}
	
	inode_offset = (_inode_seq ) * INODE_SIZE;//考虑开头的SuperBlock,inode_seq从1开始。
	
	t_inode.i_num = _inode_seq;
	t_inode.i_offset = page_offset;
	t_inode.i_father_num = father_inode_num;
	sprintf(t_inode.i_name, "%s", name);
	
	dump_inode(t_inode);
	
	ret = pwrite(fd_img, &t_inode, INODE_SIZE, inode_offset);
	if(ret != INODE_SIZE)
		return -1;
	else
		return 0;
}


unsigned int data_page_offset = 3;  //data start frome 4th page
unsigned int inode_seq  = 0;
static int process_directory(int parent, const char *path)
{

	DIR *dir;
	struct dirent *entry;
	
	inode_seq++;	//inode序号从 1 开始
	
	dir = opendir(path); //打开目录句柄！
	if(!dir)
	{
		warn("opendir");
	}
	else
	{
		while((entry = readdir(dir)) != NULL) // 循环读取目录内的文件和目录
		{
		
			/* Ignore . and .. */
			if(strcmp(entry->d_name,".") &&
			   strcmp(entry->d_name,".."))
 			{
 				char full_name[500];
				struct stat stats; //文件相关信息结构体
				
				int newObj;
				//full_name中写入文件名
				if (snprintf(full_name,sizeof(full_name),"%s/%s",path,entry->d_name) >= (int)sizeof(full_name))
				{
					error = -1;
					continue;
				}
				
				if (lstat(full_name,&stats) < 0) //获取文件相关信息
				{
					warn("lstat");
					continue;
				}
				
				if( S_ISREG(stats.st_mode) ||
				    S_ISDIR(stats.st_mode) )  //检查文件类型，是不是能够处理的类型
				{
					
					
					if(S_ISREG(stats.st_mode))
					{
						//如果是一个普通文件。
						int h;
						unsigned char bytes[PAGE_SIZE];
						int n_bytes;
						unsigned int first_page;
						
						data_page_offset++;
						h = open(full_name,O_RDONLY);
						if(h >= 0)
						{
							first_page = data_page_offset;
							memset(bytes,0xff,sizeof(bytes));
							while((n_bytes = read(h,bytes,sizeof(bytes))) > 0)//读回来一个chunk数据，
							{	
								write_page(bytes, data_page_offset, n_bytes);
								memset(bytes,0xff,sizeof(bytes));
								data_page_offset++;
							}
							if(n_bytes < 0) 
							   warn("read:");
							   
							printf("%d data chunks written\n", data_page_offset);
							close(h);
							
							write_inode(inode_seq, &stats, first_page, parent, entry->d_name);
							inode_seq++;  //inode序号递加
						}
						else
						{
							warn("open");
						}	
						
					}
					else if(S_ISDIR(stats.st_mode))
					{
						printf("directory\n");
						if (write_inode(inode_seq, &stats, 0, parent, entry->d_name) == 0) {
							
							process_directory(inode_seq,full_name);
						}
					}
					
				}
				else
				{
					fprintf(stderr, "%s: unhandled type\n", full_name);
					error |= 2;
					savedErrno = EINVAL;
				}
			}
		}
		closedir(dir);
	}
	
	return 0;

}

int fill_super_blcok()
{
	int ret;
	struct daifs_super_block t_sb;
	
	memset(t_sb.s_name, 0, sizeof(t_sb.s_name));
	
	t_sb.s_word0 = 0x2d246169;
	t_sb.s_word1 = 0x2d66732d;
	sprintf(t_sb.s_name, "%s", "daikunhai");
	
	ret = pwrite(fd_img, &t_sb, SUPER_BLOCK_SIZE, 0);
	if(ret != SUPER_BLOCK_SIZE)
		return -1;
	else
		return 0;
	
}

int main(int argc, char *argv[])
{
	struct stat stats;
	
	printf("mkdaifsimage: image building tool for DAIFS built "__DATE__"\n");
	
	if(argc < 3)
	{
		printf("usage: mkdaifsimage dir image_file [convert]\n");
		printf("           dir        the directory tree to be converted\n");
		printf("           image_file the output file to hold the image\n");
		exit(1);
	}
	
	if(stat(argv[1],&stats) < 0)
	{
		printf("Could not stat %s\n",argv[1]);
		exit(1);
	}
	
	if(!S_ISDIR(stats.st_mode))
	{
		printf(" %s is not a directory\n",argv[1]);
		exit(1);
	}
	
	fd_img = open(argv[2],O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
	
	
	if(fd_img < 0)
	{
		printf("Could not open output file %s\n",argv[2]);
		exit(1);
	}
	
	printf("Write super block...\n");
	fill_super_blcok();
	
	printf("Processing directory %s into image file %s\n",argv[1],argv[2]);
	process_directory(0, argv[1]);//  #define YAFFS_OBJECTID_ROOT 1  <in yaffs_guts.h>

	close(fd_img);
	
	if(error)
	{
		errno = savedErrno;
		perror("operation incomplete");
	}
	else
	{
		printf("Operation complete.\n"
		       "%d objects in directories\n"
		       "%d Flash pages\n",inode_seq, data_page_offset);
	}
	
	exit(error);
}




