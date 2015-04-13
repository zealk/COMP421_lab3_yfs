#ifndef __YFS_H__
#define __YFS_H__

#include <comp421/filesystem.h>
#include "fscache.h"

#define OPEN 1
#define CLOSE 2
#define CREATE 3
#define READ 4
#define WRITE 5
#define SEEK 6
#define LINK 7
#define UNLINK 8
#define SYMLINK 9
#define READLINK 10
#define MKDIR 11
#define RMDIR 12
#define STAT 13
#define SYNC 14
#define SHUTDOWN 15

#define ERROR -1

#define PARSE_END_PATH 1
#define PARSE_SUCCESS 2

#define INODE_PER_BLOCK BLOCKSIZE / INODESIZE;
#define DIR_ENTRY_PER_BLOCK BLOCKSIZE / sizeof(struct dir_entry)

struct yfs_msg_sent{
	int type;
	int pid;
	int data1;
	int data2;
	void* addr1;
	void* addr2;
};

struct yfs_msg_sent_seek{
	int type;
	int pid;
	int data1;
	int data2;
	int data3;
	int data4;
	void* addr2;
};

struct yfs_msg_returned{
	int data1;
	int data2;
	int data3;
	int data4;
	void* addr1;
	void* addr2;
};

struct fs_header header;

Cache* inode_cache;

Cache* block_cache;

int yfs_open(void* addr, int pid);
short ParsePathName(short inum, char* pathname);
int ParseComponent(char* pathname, char* component_name, int index);
short ParseSymbolicLink(struct inode* inode, char* component_name);
short GetInumByComponentName(struct inode* inode, char* component_name);

struct inode* GetInodeByInum(short inum);
void* GetBlockByBnum(short bnum);
void* GetBlockByInum(short inum);
void WriteBackInode(CacheNode* inode);
void WriteBackBlock(CacheNode* block);
short GetBlockNumFromInodeNum(short inum);

#endif