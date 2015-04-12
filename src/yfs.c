#include <comp421/filesystem.h>
#include <stdio.h>
#include <stdlib.h>
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
		 		break;
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