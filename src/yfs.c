#include <comp421/filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/yfs.h"

int main(int argc, char* argv[]){

	if(Register(FILE_SERVER) != 0){
		printf("ERROR : Cannot register yfs as file server!\n");
		return -1;
	}

	if(Fork() == 0)
		Exec(argv[1], argv + 1);



	while(1){
		 struct yfs_msg_sent* msg = malloc(sizeof(struct yfs_msg_sent));
		 int msg_type;
		 int pid;
		 
		 Receive(msg);
		 msg_type = msg->type;
		 pid = msg->pid;

		 switch(msg_type){
		 	case OPEN :
		 	{
		 		int fd = yfs_open(msg->addr1, pid);
		 		struct yfs_msg_returned* msg_returned = (struct yfs_msg_returned* )msg;
		 		msg_returned->data1 = fd;
		 		Reply(msg_returned, pid);
		 		break;
		 	}
		 	case CLOSE :
		 		break;
		 	case CREATE :
		 		break;
		 	case READ :
		 		break;
		 	case WRITE :
		 		break;
		 	case SEEK :
		 		break;
		 	case UNLINK :
		 		break;
		 	case SYMLINK :
		 		break;
		 	case READLINK :
		 		break;
		 	case MKDIR :
		 		break;
		 	case RMDIR :
		 		break;
		 	case STAT :
		 		break;
		 	case SYNC :
		 		break;
		 	case SHUTDOWN :
		 		break;
		 	default :
		 		printf("ERROR : Invalid message type!\n");
		 		break;
		 }

		free(msg); 
	}

	return 0;
}

int yfs_open(void* addr, int pid){
	
	// copy the file name from client
	void* pathname = malloc(sizeof(MAXPATHNAMELEN));
	
	if(CopyFrom(pid, pathname, addr, MAXPATHNAMELEN) != 0)
		return ERROR;

	// parse the file name 
	yfs_parse_name(1, pathname, 0);

	return 0;
}

int yfs_parse_name(short inum, void* pathname, int pathname_index){

	// get the inode from inum
	int block_num = 1 + inum / INODE_PER_BLOCK;
	int inode_index = inum % INODE_PER_BLOCK;

	void* sector = malloc(sizeof(SECTORSIZE));
	if(ReadSector(block_num, sector) != 0){
		free(sector);
		return ERROR;
	}

	struct inode_block* ib = (struct inode_block* )sector;
	struct inode curr_inode = ib->inodes[inode_index];

	if(curr_inode.type == INODE_FREE || (curr_inode.type == INODE_REGULAR && ((char* )pathname)[pathname_index] != NULL)){
		free(sector);
		return ERROR;
	}

	/* parse the name */
	// parse out the component and update the pathname for next resursive call
	void* component_name;
	int ret = helper_parse_component(pathname, component_name, &pathname_index);
	
	if(ret == ERROR){
		free(sector);
		return ERROR;
	}
	
	if(ret == PARSE_END_PATH){
		free(sector);
		return inum;
	}
	
	// else ret == PARSE_SUCCESS
	// find the component in the inode dir_entry
	if(curr_inode.type == INODE_DIRECTORY){
		// right now only care about direct blocks
		int i;
		int count = 0;
		bool flag_found = false;
		void* buf = malloc(sizeof(SECTORSIZE)); // NEED TO FREE LATER
		int total_number_dir_entry = curr_inode.size / sizeof(struct dir_entry);

		for(i = 0; i < NUM_DIRECT; i ++){
			int curr_block_num = curr_inode.direct[i];
			if(curr_block_num == 0){
				free(sector);
				free(buf);
				return ERROR;
			}
			if(ReadSector(curr_block_num, buf) != 0){
				free(sector);
				free(buf);
				return ERROR;
			}

			struct dir_entry_block* deb = (struct dir_entry_block* )buf;
			int j;
			for(j = 0; (j < DIR_ENTRY_PER_BLOCK) && (count < total_number_dir_entry); j ++){
				count ++;
				if(strcmp(deb->dir_entries[j].name, component_name) != 0){
					continue;
				}
				else{
					int next_inum = deb->dir_entries[i].inum;
					free(sector);
					free(buf);
					return yfs_parse_name(next_inum, pathname, pathname_index);
				}
			}
		}
		return ERROR; // have not found the file in the current directory
		// TODO : CARE ABOUT INDIRECT BLOCKS
	}

	// TODO : SYMBOLIK LINK
	if(curr_inode.type == INODE_SYMLINK){

	}
}

int helper_parse_component(void* pathname, void* component_name, int* pathname_index){
	if(pathname == NULL)
		return ERROR;

	//int slash_start = 0;
	//int slash_end = 0;
	int component_start = 0;
	int component_end = 0;
	bool flag_start = false;
	bool flag_slash_end = false;
	bool flag_null_end = false;

	char curr_char = ((char* )pathname)[0];
	if(curr_char == NULL)
		return PARSE_END_PATH;
	int i;
	for(i = 0;;i ++){
		curr_char = ((char* )pathname)[i];
		
		if(curr_char == '\\'){
			if(flag_start == false){
				continue;
			}
			else{
				flag_slash_end = true;
				break;
			}
		}
		else if(curr_char == NULL){
			if(flag_start == false){
				return PARSE_END_PATH;
			}
			else{
				flag_null_end = true;
				break;
			}
		}
		else{
			if(flag_start == false){
				flag_start = true;
				component_start = i;
				continue;
			}
			else{
				continue;
			}
		}
	}

	// save component name
	component_end = i; 
	component_name = malloc(sizeof(i)); // NEED TO FREE LATER
	memcpy(component_name, pathname, i - 1);
	((char* )component_name)[i - 1] = NULL;

	// update pathname
	if(flag_null_end == true){
		*pathname_index = *pathname_index + i;
	}
	else{
		*pathname_index = *pathname_index + i + 1;
	}

	return PARSE_SUCCESS;
}




























