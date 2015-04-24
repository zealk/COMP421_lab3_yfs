#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "include/yfs.h"

int curr_inum = 1;

typedef struct file_info {
	bool valid;
	int curr_seek_pos;
	int inum;
} FileInfo;

FileInfo opened_files[MAX_OPEN_FILES];

int Open(char* pathname){
	if (strlen(pathname) > MAXPATHNAMELEN) {
		return ERROR;
    }

    /* Check available slot in opened_files */
    int i;
    for (i = 0; i < MAX_OPEN_FILES; ++i) {
        if (!opened_files[i].valid) {
            break;
        }
    }

    if (i == MAX_OPEN_FILES) {
        return ERROR;
    }
    int fd = i;

	Message* msg = calloc(1, sizeof(Message));
	msg->type = OPEN;
    msg->data1 = curr_inum;
	msg->addr1 = (void*)pathname;

	if (Send((void*)msg, -FILE_SERVER) == ERROR) {
        free(msg);
		return ERROR;
    }

    opened_files[fd].valid = true;
    opened_files[fd].curr_seek_pos = 0;
    opened_files[fd].inum = msg->data1;
    
    free(msg);    
    return fd;
}

int Close(int fd){
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return ERROR;
    }

    if (!opened_files[fd].valid) {
        return ERROR;
    }

    opened_files[fd].valid = false;
	return 0;
}

int Create(char* pathname){
	if (strlen(pathname) > MAXPATHNAMELEN) {
		return ERROR;
    }
	
	Message* msg = malloc(sizeof(Message));
	msg->type = CREATE;
	msg->data1 = curr_inum;
	msg->addr1 = (void*)pathname;

	if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
		return ERROR;
    }

    int i;
    for(i = 0; i < MAX_OPEN_FILES; ++i){
    	if(opened_files[i].valid == false){
    		opened_files[i].valid = true;
    		opened_files[i].curr_seek_pos = 0;
    		opened_files[i].inum = msg->data1;
    		break;
    	}
    }
    
    free(msg);
    if(i == MAXPATHNAMELEN){
    	return ERROR;
    }

	return i;
}

int Read(int fd, void* buf, int size){
	if (fd < 0 || fd >= MAX_OPEN_FILES || buf == NULL || size < 0) {
        return ERROR;
    }

    if (!opened_files[fd].valid) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = READ;
    msg->data1 = opened_files[fd].inum;
    msg->data2 = size;
    msg->data3 = opened_files[fd].curr_seek_pos;
    msg->addr1 = buf;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }

    int ret = msg->type;
    free(msg);
    if (ret == ERROR) {
        return ERROR;
    }

    opened_files[fd].curr_seek_pos += ret;
    return ret;
}

int Write(int fd, void* buf, int size){
    if (fd < 0 || fd >= MAX_OPEN_FILES || buf == NULL || size < 0) {
        return ERROR;
    }

    if (!opened_files[fd].valid) {
        return ERROR;
    }

	Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = WRITE;
    msg->data1 = opened_files[fd].inum;
    msg->data2 = size;
    msg->data3 = opened_files[fd].curr_seek_pos;
    msg->addr1 = buf;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }

    int ret = msg->type;
    free(msg);
    if (ret == ERROR) {
        return ERROR;
    }
    
    return ret;
}

int Seek(int fd, int offset, int whence){
    if (fd < 0 || fd >= MAX_OPEN_FILES || !opened_files[fd].valid) {
        return ERROR;
    }

    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = SEEK;
    msg->data1 = opened_files[fd].inum;
    msg->data2 = offset;
    msg->data3 = whence;
    msg->addr1 = &(opened_files[fd].curr_seek_pos);

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }

    int seek_pos = msg->type;
    free(msg);
    if (seek_pos == ERROR) {
        return ERROR;
    }
    
    opened_files[fd].curr_seek_pos = seek_pos;
    return seek_pos;
}

int Link(char* oldname, char* newname){
	if (oldname == NULL || strlen(oldname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    if (newname == NULL || strlen(newname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = LINK;
    msg->data1 = curr_inum;
    msg->addr1 = oldname;
    msg->addr2 = newname;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }

    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    free(msg);
    return 0;
}

int Unlink(char* pathname){
	if (pathname == NULL || strlen(pathname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = UNLINK;
    msg->data1 = curr_inum;
    msg->addr1 = pathname;

    Send((void*)msg, -FILE_SERVER);
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    free(msg);
    return 0;
}

int SymLink(char* oldname, char* newname){
	if (oldname == NULL || strlen(oldname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    if (newname == NULL || strlen(newname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = UNLINK;
    msg->data1 = curr_inum;
    msg->addr1 = oldname;
    msg->addr2 = newname;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }
    
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    free(msg);
    return 0;
}

int ReadLink(char* pathname, char* buf, int len) {
	if (pathname == NULL || strlen(pathname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    if (buf == NULL || len < 0) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = READLINK;
    msg->data1 = curr_inum;
    msg->addr1 = pathname;
    msg->addr2 = buf;
    msg->data2 = len;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }
    
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    int ret = msg->type;
    free(msg);
    return ret;
}

int MkDir(char* pathname) {
	if (pathname == NULL || strlen(pathname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = MKDIR;
    msg->data1 = curr_inum;
    msg->addr1 = pathname;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }
    
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    free(msg);
    return 0;
}

int RmDir(char* pathname) {
	if (pathname == NULL || strlen(pathname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = RMDIR;
    msg->data1 = curr_inum;
    msg->addr1 = pathname;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }
    
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    free(msg);
    return 0;
}

int ChDir(char* pathname) {
	if (pathname == NULL || strlen(pathname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = RMDIR;
    msg->data1 = curr_inum;
    msg->addr1 = pathname;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }
    
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    curr_inum = msg->data1;
    free(msg);
    return 0;
}

int Stat(char* pathname, struct Stat* statbuf) {
    if (pathname == NULL || strlen(pathname) > MAXPATHNAMELEN) {
        return ERROR;
    }

    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = STAT;
    msg->data1 = curr_inum;
    msg->addr1 = (void*)pathname;
    msg->addr2 = (void*)statbuf;

    if (Send(msg, -FILE_SERVER) == ERROR) {
        free(msg);
        return ERROR;
    }
    
    if (msg->type == ERROR) {
        free(msg);
        return ERROR;
    }

    statbuf->inum = msg->type;
    statbuf->type = msg->data1;
    statbuf->size = msg->data2;
    statbuf->nlink = msg->data3;

    free(msg);
    return 0;
}

int Sync(void) {
	Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = SYNC;

    Send((void*)msg, -FILE_SERVER);

    free(msg);
    return 0;
}

int Shutdown(void) {
    Message* msg = (Message*)calloc(1, sizeof(Message));
    msg->type = SHUTDOWN;

    Send((void*)msg, -FILE_SERVER);

    free(msg);
	return 0;
}