#include "../include/yfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int main(int argc, char* argv[]) {
	printf("%d\n", SECTORSIZE);

	if(Register(FILE_SERVER) != 0){
		printf("ERROR : Cannot register yfs as file server!\n");
		return -1;
	}

	if(Fork() == 0)
		Exec(argv[1], argv + 1);

	/* Init file system header */



	/* Init cache */
	inode_cache = InitCache(INODE_CACHESIZE);
	block_cache = InitCache(BLOCK_CACHESIZE);

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
	ParsePathName(1, pathname);

	return 0;
}

short ParsePathName(short inum, char* pathname){
	if (pathname == NULL) {
		return ERROR;
	}

	/* Parse absolute pathname */
	if (pathname[0] == '/') {
		inum = 1;
	}

	int pathname_index = 0;
	char* component_name = NULL;
	pathname_index = ParseComponent(pathname, component_name, pathname_index);

	while (component_name != NULL) {
		/* Check the current inode */
		struct inode* inode = GetInodeByInum(inum);
		if (inode->type == INODE_FREE || inode->type == INODE_REGULAR) {
			return ERROR;
		}

		/* Get child inode number by component name */
		if (inode->type == INODE_DIRECTORY) {
			inum = GetInumByComponentName(inode, component_name);

		/* Get inode number by symbolic link recursively */
		} else {
			inum = ParseSymbolicLink(inode, component_name);
		}

		/* Release component name */
		free(component_name);
		component_name = NULL;

		/* Get next component name */
		pathname_index = ParseComponent(pathname, component_name, pathname_index);
	}

	return inum;
}

int ParseComponent(char* pathname, char* component_name, int index) {
	/* Jump continuous slash symbol */
	while (pathname[index] == '/') {
		++index;
	}

	int component_start = index;

	/* Jump valid characters */
	while (pathname[index] != '/' && pathname[index] != '\0') {
		++index;
	}

	int component_end = index;

	/* Parse nothing */
	if (component_start == component_end) {
		return index;
	}

	/* Parse something */
	component_name = (char*)calloc(component_end - component_start + 1, sizeof(char));
	memcpy(component_name, pathname + component_start, component_end - component_start);

	return index;
}

short GetInumByComponentName(struct inode* inode, char* component_name) {
	int inum = -1;
	int dir_entry_count = 0;
	int total_dir_entry = inode->size / sizeof(struct dir_entry);

	int i, j;
	for (i = 0; i < NUM_DIRECT; ++i) {
		short bnum = inode->direct[i];
		if (bnum == 0) {
			return inum;
		}

		void* block = GetBlockByBnum(bnum);
		if (block == NULL) {
			return inum;
		}

		for (j = 0; j < DIR_ENTRY_PER_BLOCK && dir_entry_count < total_dir_entry; ++j, ++dir_entry_count) {
			struct dir_entry* entry = (struct dir_entry*)block + j;
			char* dir_entry_name = NULL;
			if (entry->name[DIRNAMELEN] != '\0') {
				dir_entry_name = (char*)calloc(DIRNAMELEN + 1, sizeof(char));
			} else {
				dir_entry_name = (char*)calloc(sizeof(entry->name) + 1, sizeof(char));
			}

			if (strcmp(component_name, dir_entry_name) != 0) {
				continue;
			}

			inum = entry->inum;
			return inum;
		}
	}

	return inum;
}

short ParseSymbolicLink(struct inode* inode, char* component_name) {

}

struct inode* GetInodeByInum(short inum) {
	if (inum <= 0 || inum > header.num_inodes) {
		return NULL;
	}

	/* Get inode from cache */
	struct inode* inode = (struct inode*)GetItemFromCache(inode_cache, inum);
	if (inode == NULL) {
		/* Get inode from block */
		void* block = GetBlockByInum(inum);
		if (block == NULL) {
			return NULL;
		}

		struct inode* inode = (struct inode*)calloc(1, sizeof(struct inode));
		short offset = inum % INODE_PER_BLOCK;
		memcpy(inode, (struct inode*)block + offset, sizeof(struct inode));

		/* Cache inode */
		CacheNode* inode_cache_node = PutItemInCache(inode_cache, inum, inode);
		if (inode_cache_node != NULL) {
			WriteBackInode(inode_cache_node);
		}
	}

	return inode;
}

void* GetBlockByBnum(short bnum) {
	if (bnum <= 0 && bnum >= header.num_blocks) {
		return NULL;
	}

	void* block = GetItemFromCache(block_cache, bnum);
	if (block == NULL) {
		block = malloc(SECTORSIZE);
		if (ReadSector(bnum, block) != 0) {
			printf("Read Sector #%d failed\n", bnum);
			free(block);
			return NULL;
		}

		/* Cache block */
		CacheNode* block_cache_node = PutItemInCache(block_cache, bnum, block);
		if (block_cache_node != NULL) {
			WriteBackBlock(block_cache_node);
		}
	}

	return block;
}

void* GetBlockByInum(short inum) {
	if (inum <= 0 || inum > header.num_inodes) {
		return NULL;
	}

	short bnum = GetBlockNumFromInodeNum(inum);

	return GetBlockByBnum(bnum);
}

void WriteBackInode(CacheNode* inode) {
	void* block = GetBlockByInum(inode->key);
	short bnum = GetBlockNumFromInodeNum(inode->key);
	if (block == NULL) {
		return;
	}

    /* Save inode in the block and set dirty bit for that block */
    short offset = inode->key % INODE_PER_BLOCK;
    memcpy((struct inode*)block + offset, (struct inode*)(inode->value), sizeof(struct inode));
    SetDirty(block_cache, bnum);

    free(inode->value);
    free(inode);
}

void WriteBackBlock(CacheNode* block) {
    int code = WriteSector(block->key, block->value);
    /* Maybe it needs to do other things here */
    if (code != 0) {
        printf("Write Sector #%d failed\n", block->key);
    }

    free(block->value);
    free(block);
}

short GetBlockNumFromInodeNum(short inum) {
    return 1 + inum / INODE_PER_BLOCK;
}