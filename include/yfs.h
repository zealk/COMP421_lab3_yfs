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

#define ERROR -1

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

#endif