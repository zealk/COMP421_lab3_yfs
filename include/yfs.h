#ifndef __YFS_H__
#define __YFS_H__

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

struct yfs_msg{
	int type;
	int data;
	char pathname[16];
	void* ptr;
};

#endif