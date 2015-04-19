#include "../include/yfs.h"
#include <stdlib.h>
#include <comp421/yalnix.h>

int YfsOpen(struct abstract_msg* msg) {
    // copy the file name from client
    struct yfs_msg_open* msg_in = (struct yfs_msg_open* )msg;
    char* pathname = malloc(sizeof(MAXPATHNAMELEN));
    int pid = msg_in->pid;
    int curr_inum = msg_in->curr_inum;
    char* addr = msg_in->pathname;
    
    if(CopyFrom(pid, pathname, addr, MAXPATHNAMELEN) == ERROR)
        return ERROR;


    // parse the file name 
    int inum = ParsePathName(curr_inum, pathname);
    if(inum == ERROR)
    	return ERROR;
   

	struct yfs_msg_returned* msg_returned = (struct yfs_msg_returned* )msg;
	msg_returned->type = OPEN;
	msg_returned->data1 = inum;
	Reply(msg_returned, pid);

	// free the data
	free(pathname);

    return 0;
}

int YfsCreate(struct abstract_msg* msg) {
    // copy the file name from client
    struct yfs_msg_open* msg_in = (struct yfs_msg_open* )msg;
    char* pathname = malloc(sizeof(MAXPATHNAMELEN));
    int pid = msg_in->pid;
    int curr_inum = msg_in->curr_inum;
    char* addr = msg_in->pathname;
    
    if(CopyFrom(pid, pathname, addr, MAXPATHNAMELEN) == ERROR)
        return ERROR;

    // get the path before the last component in the pathname, e.g " c"
    // will be "a/b//"
    int i;
    int filename_index = 0;

    if(pathname[strlen(pathname) - 1] == '/')
    	return ERROR;

    for(i = strlen(pathname) - 2; i >= 1 ; i --){	
		if(pathname[i] == '/'){
			filename_index = i + 1;
			break;
		}
    }
    
    char* new_pathname = malloc(sizeof(filename_index + 1));
    char* filename = malloc(sizeof(strlen(pathname) - filename_index));

    memcpy(new_pathname, pathname, filename_index + 1);
    memcpy(filename, pathname, strlen(pathname) - filename_index);

    int inum = ParsePathName(curr_inum, new_pathname);

    if(inum == ERROR)
    	return ERROR;

    if(num_free_inodes == 0)
    	return ERROR;

    if(num_free_blocks == 0)
    	return ERROR;

    // create another dir_entry in current inode's block
    int block_with_dir_entry;
    struct dir_entry* new_dir_entry = FindAvailableDir_Entry(inum, &block_with_dir_entry);

    // update the inum
    // update hte name

}






