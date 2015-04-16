#include <comp421/iolib.h>
#include <stdlib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "include/yfs.h"

int Open(char* pathname){

	struct yfs_msg_sent* msg = malloc(sizeof(struct yfs_msg_sent));
	msg->type = OPEN;
	msg->pid = GetPid();
	msg->addr1 = pathname;

	if(Send(msg, -FILE_SERVER) != ERROR){
		// TODO : need to copy from the server
		struct yfs_msg_returned* msg_returned = (struct yfs_msg_returned* )msg;
		return msg->data1;
	}
	else
		return ERROR;
}

int Close(int fd){
	return 0;
}

int Create(char* pathname){
	return 0;
}

int Read(int fd, void* buf, int size){
	return 0;
}

int Write(int fd, void* buf, int size){
	return 0;
}

int Seek(int fd, int offset, int whence){
	return 0;
}

int Link(char* oldname, char* newname){
	return 0;
}

int Unlink(char* pathname){
	return 0;
}

int SymLink(char* oldname, char* newname){
	return 0;
}

int ReadLink(char* pathname, char* buf, int len){
	return 0;
}

int MkDir(char* pathname){
	return 0;
}

int RmDir(char* pathname){
	return 0;
}

int ChDir(char* pathname){
	return 0;
}

int Stat(char* pathname, struct Stat* statbuf){
	return 0;
}

int Sync(void){
	return 0;
}

int Shutdown(void){
	return 0;
}