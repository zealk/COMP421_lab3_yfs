#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "include/yfs.h"

int curr_inum = 1;

typedef struct FileInfo {
	bool valid;
	//int fd;
	int curr_seek_pos;
	int inum;
} FileInfo;

FileInfo opened_files[MAX_OPEN_FILES];

int Open(char* pathname){

	if(strlen(pathname) > MAXPATHNAMELEN)
		return ERROR;

	struct yfs_msg_open* msg = malloc(sizeof(struct yfs_msg_open));
	msg->type = OPEN;
	msg->pid = GetPid();
	msg->curr_inum = curr_inum;
	msg->pathname = pathname;

	if(Send(msg, -FILE_SERVER) == ERROR)
		return ERROR;

	// struct yfs_msg_open* msg_returned = (struct yfs_msg_open* )msg;
	int inum = msg->data1;

	int fd;
    int i;
    for(i = 0; i < MAX_OPEN_FILES; i ++){
    	if(opened_files[i].valid == false)
    		continue;

    	if(opened_files[i].inum == inum){
    		fd = i;
    		break;
    	}
    }

    free(msg);

    if(i == MAX_OPEN_FILES)
    	return ERROR;
    else
    	return fd;

}

int Close(int fd){

	return 0;
}

int Create(char* pathname){

	if(strlen(pathname) > MAXPATHNAMELEN)
		return ERROR;
	
	struct yfs_msg_open* msg = malloc(sizeof(struct yfs_msg_open));
	msg->type = CREATE;
	msg->pid = GetPid();
	msg->curr_inum = curr_inum;
	msg->pathname = pathname;

	if(Send(msg, -FILE_SERVER) == ERROR)
		return ERROR;

	int inum = msg->data1;
    int i;
    for(i = 0; i < MAX_OPEN_FILES; i ++){
    	if(opened_files[i].valid == false){
    		opened_files[i].valid = true;
    		opened_files[i].curr_seek_pos = 0;
    		opened_files[i].inum = inum;
    		break;
    	}
    }
    
    if(i == MAXPATHNAMELEN){
    	return ERROR;
    }

    free(msg);

	return i;
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
	if (strlen(oldname) > MAXPATHNAMELEN || strlen(newname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    yfs_msg_link* msg = (yfs_msg_link*)calloc(1, sizeof(yfs_msg_link));
    msg->type = LINK;
    msg->pid = GetPid();
    msg->inum = curr_inum;
    msg->oldname = oldname;
    msg->newname = newname;

    Send(msg, msg->pid);
    if (msg->type == ERROR) {
        return ERROR;
    }

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