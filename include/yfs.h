#ifndef __YFS_H__
#define __YFS_H__

#include <comp421/filesystem.h>
#include "fscache.h"

#define OPEN 1
#define CREATE 2
#define READ 3
#define WRITE 4
#define SEEK 5
#define LINK 6
#define UNLINK 7
#define SYMLINK 8
#define READLINK 9
#define MKDIR 10
#define RMDIR 11
#define CHDIR 12
#define STAT 13
#define SYNC 14
#define SHUTDOWN 15

#define INODE_PER_BLOCK BLOCKSIZE / INODESIZE;
#define DIR_ENTRY_PER_BLOCK BLOCKSIZE / sizeof(struct dir_entry)

typedef struct message {
	int type;
    int data1;
	int data2;
    int data3;
	void* addr1;
	void* addr2;
} Message;

struct fs_header header;

Cache* inode_cache;

Cache* block_cache;

/* True if unused, otherwise false */
bool* free_inodes;

int num_free_inodes;

bool* free_blocks;

int num_free_blocks;

void YfsOpen(Message* msg, int pid);
void YfsCreate(Message* msg, int pid);
void YfsRead(Message* msg, int pid);
void YfsWrite(Message* msg, int pid);
void YfsSeek(Message* msg, int pid);
void YfsLink(Message* msg, int pid);
void YfsUnlink(Message* msg, int pid);
void YfsSymLink(Message* msg, int pid);
void YfsReadLink(Message* msg, int pid);
void YfsMkDir(Message* msg, int pid);
void YfsRmDir(Message* msg, int pid);
void YfsChDir(Message* msg, int pid);
void YfsStat(Message* msg, int pid);
void YfsSync(Message* msg, int pid);
void YfsShutDown(Message* msg, int pid);

int InitFileSystem();
int ParsePathName(int inum, char* pathname);
int ParseComponent(char* pathname, char* component_name, int index);
int ParseSymbolicLink(struct inode* inode, int traverse_count);
int GetInumByComponentName(struct inode* inode, char* component_name);

struct inode* GetInodeByInum(int inum);
void* GetBlockByBnum(int bnum);
void* GetBlockByInum(int inum);
void WriteBackInode(CacheNode* inode);
void WriteBackBlock(CacheNode* block);
int GetBlockNumFromInodeNum(int inum);
int GetBnumFromIndirectBlock(int indirect_bnum, int index);
int GetBnumBySeekPosition(struct inode* inode, int seek_pos);
int AllocateBlockInInode(struct inode* inode, int inum);

int CountDirEntry(struct inode* dir_inode, int dir_inum);
int DeleteDirEntry(struct inode* dir_inode, int dir_inum, int inum);
int CreateDirEntry(struct inode* dir_inode, int dir_inum, int inum, char* name);
int GetFileNameIndex(char* pathname);
int ParsePathDir(int inum, char* pathname);

int FindFreeBlock(void);
void RecycleFreeBlock(int bnum);
int FindFreeInode(void);
void RecycleFreeInode(int inum);
void SyncInodeCache();
void SyncBlockCache();

int RecycleBlocksInInode(int inum);

#endif