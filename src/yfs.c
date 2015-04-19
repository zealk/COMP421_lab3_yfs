#include "../include/yfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char* argv[]) {
	if (InitFileSystem() == ERROR) {
		return ERROR;
	}

	if (Register(FILE_SERVER) == ERROR) {
		printf("ERROR : Cannot register yfs as file server!\n");
		return ERROR;
	}

	if(Fork() == 0) {
		Exec(argv[1], argv + 1);
	}

	while(1){
		 
		 struct abstract_msg* msg = malloc(sizeof(struct abstract_msg));
		 Receive(msg);
		 int msg_type = msg->type;

		 switch(msg_type){
		 	case OPEN :
		 	{
		 		if(YfsOpen(msg) == ERROR)
		 			return ERROR;
		 		break;
		 	}
		 	case CLOSE :
		 		break;
		 	case CREATE :
		 	{
		 		if(YfsCreate(msg) == ERROR)
		 			return ERROR;
		 		break;
		 	}
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

int InitFileSystem() {
	/* Init cache */
	inode_cache = InitCache(INODE_CACHESIZE);
	block_cache = InitCache(BLOCK_CACHESIZE);

	/* Init file system header */
	void* second_block = GetBlockByBnum(1);
	if (second_block == NULL) {
		printf("Read Sector #1 failed\n");
		return ERROR;
	}

	header.num_blocks = ((struct fs_header*)second_block)->num_blocks;
	header.num_inodes = ((struct fs_header*)second_block)->num_inodes;

	/* Never care about index 0 in free blocks */
	free_blocks = (bool*)calloc(header.num_blocks + 1, sizeof(bool));
	/* Never care about index 0 in free_inodes */
	free_inodes = (bool*)calloc(header.num_inodes + 1, sizeof(bool));

	/* Initialize all_inodes array */
	int i;
	num_free_inodes = 0;
	for (i = 1; i < header.num_inodes + 1; ++i) {
		struct inode* inode = GetInodeByInum(i);
		if (inode == NULL) {
			printf("Can't get inode #%d\n", i);
			return ERROR;
		}

		if (inode->type == INODE_FREE) {
			free_inodes[i] = true;
			++num_free_inodes;
		}
	}

	/* Initialize free_blocks array based on free_inodes array */
	num_free_blocks = header.num_blocks - (header.num_inodes + 1) / INODE_PER_BLOCK;
	i = 1 + (header.num_blocks + 1) / INODE_PER_BLOCK;
	for (; i <= header.num_blocks; ++i) {
		free_blocks[i] = true;
	}

	for (i = 1; i < header.num_inodes + 1; ++i) {
		/* Traverse all free inodes */
		if (!free_inodes[i]) {
			struct inode* inode = GetInodeByInum(i);
			if (inode == NULL) {
				printf("Can't get inode #%d\n", i);
				return ERROR;
			}

			/* Check direct block */
			int j;
			for (j = 0; j < NUM_DIRECT; ++j) {
				if (inode->direct[j] == 0) {
					break;
				}

				free_blocks[inode->direct[j]] = false;
				--num_free_blocks;
			}

			/* Check indirect block */
			if (inode->indirect > 0) {
				/* File size should be larger than NUM_DIRECT * BLOCKSIZE + INDIRECT BLOCKSIZE */
				if (inode->size < (NUM_DIRECT + 1) * BLOCKSIZE) {
					printf("Illegal use of indirect block of inode #%d\n", i);
					return ERROR;
				}

				int indirect_block_size = inode->size - NUM_DIRECT * BLOCKSIZE;
				j = 0;
				while (indirect_block_size > 0) {
					int bnum = GetBnumFromIndirectBlock(inode->indirect, j);
					if (bnum == ERROR) {
						return ERROR;
					}

					free_blocks[bnum] = false;
					--num_free_blocks;
					indirect_block_size -= BLOCKSIZE;
				}
			}
		}
	}

	return 0;
}

int ParsePathName(int inum, char* pathname){
	if (pathname == NULL) {
		return ERROR;
	}

	/* If currenty directory inode number is invalid, no need to parse relative path nae */
	if (inum == -1 && pathname[0] != '/') {
		return ERROR;
	}

	/* Parse absolute pathname */
	if (pathname[0] == '/') {
		inum = 1;
	}

	int index = 0;
	char* component_name = NULL;
	index = ParseComponent(pathname, component_name, index);

	while (component_name != NULL) {
		/* Check the current inode */
		struct inode* inode = GetInodeByInum(inum);
		if (inode->type == INODE_FREE || (inode->type == INODE_REGULAR && pathname[index] != '\0')) {
			return ERROR;
		}

		/* Get child inode number by component name */
		if (inode->type == INODE_DIRECTORY) {
			inum = GetInumByComponentName(inode, component_name);

		/* Get inode number by symbolic link recursively */
		} else if (inode->type == INODE_SYMLINK && pathname[index] != '\0') {
			inum = ParseSymbolicLink(inode, component_name, 0);		
		}

		if (inum == ERROR) {
			return ERROR;
		}

		/* Release component name */
		free(component_name);
		component_name = NULL;

		/* Get next component name */
		index = ParseComponent(pathname, component_name, index);
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

/* Traverse all the dir_entry in a directory */
int GetInumByComponentName(struct inode* inode, char* component_name) {
	if (inode->type != INODE_DIRECTORY) {
		return ERROR;
	}

	int inum = -1;
	int total_dir_entry = inode->size / sizeof(struct dir_entry);

	int i;
	for (i = 0; i < total_dir_entry; ++i) {
		int block_index = (i * sizeof(struct dir_entry)) / BLOCKSIZE;
		int bnum;
		if (block_index < NUM_DIRECT) {
			bnum = inode->direct[block_index];
		} else {
			bnum = GetBnumFromIndirectBlock(inode->indirect, block_index - NUM_DIRECT);
		}

		void* block = GetBlockByBnum(bnum);
		if (block == NULL) {
			return ERROR;
		}

		struct dir_entry entry = ((struct dir_entry*)block)[i % DIR_ENTRY_PER_BLOCK];
		if (entry.inum > 0) {
			char* name = NULL;
			if (entry.name[DIRNAMELEN - 1] != '\0') {
				name = (char*)calloc(DIRNAMELEN + 1, sizeof(char));
				memcpy(name, entry.name, DIRNAMELEN);
			} else {
				name = (char*)calloc(strlen(entry.name) + 1, sizeof(char));
				memcpy(name, entry.name, strlen(entry.name));
			}

			if (strcmp(component_name, name) != 0) {
				free(name);
				continue;
			}

			free(name);
			return (int)(entry.inum);
		}
	}

	return inum;
}

int ParseSymbolicLink(struct inode* inode, char* component_name, int traverse_count) {
	if (inode->type != INODE_SYMLINK || traverse_count > MAXSYMLINKS) {
		return ERROR;
	}

	void* block = GetBlockByBnum(inode->direct[0]);
	if (block == NULL) {
		return ERROR;
	}

	int inum = ((struct dir_entry*)block)->inum;
	if (inum == 0) {
		return ERROR;
	}

	struct inode* child = GetInodeByInum(inum);
	if (child == NULL) {
		return ERROR;
	}

	if (child->type == INODE_SYMLINK) {
		return ParseSymbolicLink(child, NULL, ++traverse_count);
	}

	return inum;
}

struct inode* GetInodeByInum(int inum) {
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
		int offset = inum % INODE_PER_BLOCK;
		memcpy(inode, (struct inode*)block + offset, sizeof(struct inode));

		/* Cache inode */
		CacheNode* inode_cache_node = PutItemInCache(inode_cache, inum, inode);
		if (inode_cache_node != NULL && inode_cache_node->dirty) {
			WriteBackInode(inode_cache_node);
		}
	}

	return inode;
}

void* GetBlockByBnum(int bnum) {
	if (bnum <= 0 && bnum >= header.num_blocks) {
		return NULL;
	}

	void* block = GetItemFromCache(block_cache, bnum);
	if (block == NULL) {
		block = malloc(SECTORSIZE);
		if (ReadSector(bnum, block) == ERROR) {
			printf("Read Sector #%d failed\n", bnum);
			free(block);
			return NULL;
		}

		/* Cache block */
		CacheNode* block_cache_node = PutItemInCache(block_cache, bnum, block);
		if (block_cache_node != NULL && block_cache_node->dirty) {
			WriteBackBlock(block_cache_node);
		}
	}

	return block;
}

void* GetBlockByInum(int inum) {
	if (inum <= 0 || inum > header.num_inodes) {
		return NULL;
	}

	int bnum = GetBlockNumFromInodeNum(inum);

	return GetBlockByBnum(bnum);
}

void WriteBackInode(CacheNode* inode) {
	void* block = GetBlockByInum(inode->key);
	int bnum = GetBlockNumFromInodeNum(inode->key);
	if (block == NULL) {
		return;
	}

    /* Save inode in the block and set dirty bit for that block */
    int offset = inode->key % INODE_PER_BLOCK;
    memcpy((struct inode*)block + offset, (struct inode*)(inode->value), sizeof(struct inode));
    SetDirty(block_cache, bnum);

    free(inode->value);
    free(inode);
}

void WriteBackBlock(CacheNode* block) {
    /* Maybe it needs to do other things here */
    if (WriteSector(block->key, block->value) == ERROR) {
        printf("Write Sector #%d failed\n", block->key);
    }

    free(block->value);
    free(block);
}

int GetBlockNumFromInodeNum(int inum) {
    return 1 + inum / INODE_PER_BLOCK;
}

int GetBnumFromIndirectBlock(int indirect_bnum, int index) {
	if (indirect_bnum < 1 || indirect_bnum > header.num_blocks) {
		printf("Illegal indirect block number #%d\n", indirect_bnum);
		return ERROR;
	}

	void* indirect_block = GetBlockByBnum(indirect_bnum);
	if (indirect_block == NULL) {
		return ERROR;
	}

	return ((int*)indirect_block)[index];
}


struct dir_entry* FindAvailableDirEntry(int inum, int* blocknum){
	struct inode* inode = GetInodeByInum(inum);

	int i;
	for(i = 0; i < inode->size / sizeof(struct dir_entry) + 1; i ++){
		
		int block_index = i / DIR_ENTRY_PER_BLOCK;
		
		if(block_index < NUM_DIRECT){ // within the direct blocks
			int blocknumber;
			
			if(inode->direct[block_index] == 0){
				blocknumber = FindFreeBlock();	
				if(blocknumber == ERROR)
					return ERROR;
				else{
					// update the free list
					free_blocks[blocknumber] = false;
					num_free_blocks --;				
				}
			}
			else{
				blocknumber = inode->direct[block_index];
			}

			struct dir_entry* block = (struct dir_entry* )GetBlockByBnum(inode->direct[blocknumber]);

			int j;
			for(j = 0; j < DIR_ENTRY_PER_BLOCK; j ++){	
				if(block[j].inum != 0)
					continue;

				int new_inum = FindFreeInode();
				if(new_inum == ERROR)
					return ERROR;
				
				*blocknum = blocknumber;
				return &block[j];
			}
		}
		else{ // in the indirect blocks
			int indirect_blocknumber = inode->indirect;

			int* indirect_block = (struct dir_entry* )GetBlockByBnum(indirect_blocknumber);

			int j;
			for(j = 0; j < BLOCKSIZE / sizeof(int); j ++){
				int sub_indirect_blocknumber = indirect_block[j];
				
				if(sub_indirect_blocknumber == 0){
					int new_blocknum = FindFreeBlock();
					if(new_blocknum == ERROR)
						return ERROR;
					else{
						// update the free list
						free_blocks[new_blocknum] = false;
						num_free_blocks --;
						// update the block[j] list
						indirect_block[j] = new_blocknum;

						*blocknum = new_blocknum;
						return &indirect_block[j];
					}
				}

				else{
					struct dir_entry* block = (struct dir_entry* )GetBlockByBnum(sub_indirect_blocknumber);
					int k;
					for(k = 0; k < DIR_ENTRY_PER_BLOCK; k ++){	
						if(block[k].inum != 0)
							continue;

						int new_inum = FindFreeInode();
						if(new_inum == ERROR)
							return ERROR;

						*blocknum = sub_indirect_blocknumber;
						return &block[j];
					}					
				}

			}
		}

	}
}


int GetFileNameIndex(char* pathname){
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
    return filename_index;
}

int FindFreeBlock(void){
	int i;
    for (i = 1; i <= header.num_blocks; ++i) {
        if (free_blocks[i]) {
            return i;
        }
    }

    return ERROR;
}

int FindFreeInode(void){
	int i;
    for (i = 1; i <= header.num_inodes; ++i) {
        if (free_inodes[i]) {
            return i;
        }
    }

    return ERROR;
}

void SaveInode(int inum) {
    SetDirty(inode_cache, inum);
}

void SaveBlock(int bnum) {
    SetDirty(block_cache, bnum);
}

int ParsePathDir(int inum, char* pathname) {
    char dir[MAXPATHNAMELEN];
    int filename_index = GetFileNameIndex(pathname);
    if (filename_index == ERROR) {
        return ERROR;
    }

    if (filename_index == 0) {
        return inum;
    }

    memcpy(dir, pathname, filename_index);
    dir[filename_index] = '\0';
    return ParsePathName(inum, dir);
}