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
		 Message* msg = (Message*)calloc(1, sizeof(Message));
		 int pid = Receive((void*)msg);

		 switch(msg->type){
		 	case OPEN: 
		 		YfsOpen(msg, pid);
		 		break;
		 	case CREATE :
		 		YfsCreate(msg, pid);
		 		break;
		 	case READ:
		 		YfsRead(msg, pid);
		 		break;
		 	case WRITE:
		 		YfsWrite(msg, pid);
		 		break;
		 	case SEEK:
		 		YfsSeek(msg, pid);
		 		break;
		 	case UNLINK:
		 		YfsUnlink(msg, pid);
		 		break;
		 	case SYMLINK:
		 		YfsSymLink(msg, pid);
		 		break;
		 	case READLINK:
		 		YfsReadLink(msg, pid);
		 		break;
		 	case MKDIR:
		 		YfsMkDir(msg, pid);
		 		break;
		 	case RMDIR:
		 		YfsRmDir(msg, pid);
		 		break;
		 	case CHDIR:
		 		YfsChDir(msg, pid);
		 		break;
		 	case STAT:
		 		YfsStat(msg, pid);
		 		break;
		 	case SYNC:
		 		YfsSync(msg, pid);
		 		break;
		 	case SHUTDOWN:
		 		YfsShutDown(msg, pid);
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
		inum = ROOTINODE;
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
			inum = ParseSymbolicLink(inode, 0);		
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

	int total_dir_entry = inode->size / sizeof(struct dir_entry);

	int i;
	for (i = 0; i < total_dir_entry; ++i) {
		int block_index = i * sizeof(struct dir_entry) / BLOCKSIZE;
		if (block_index >= NUM_DIRECT && inode->indirect == 0) {
			return ERROR;
		}

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
			if (strncmp(component_name, entry.name, DIRNAMELEN) == 0) {
				return (int)(entry.inum);
			}
		}
	}

	return 0;
}

int ParseSymbolicLink(struct inode* inode, int traverse_count) {
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
		return ParseSymbolicLink(child, ++traverse_count);
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

int GetBnumBySeekPosition(struct inode* inode, int seek_pos) {
	int block_index = seek_pos / BLOCKSIZE;

	/* Get Bnum from direct block */
	if (block_index < NUM_DIRECT) {
		if (inode->direct[block_index] == 0) {
			return ERROR;
		}

		return inode->direct[block_index];
	}

	/* Get Bnum from indirect block */
	if (inode->indirect == 0) {
		return ERROR;
	}

	int bnum = GetBnumFromIndirectBlock(inode->indirect, block_index - NUM_DIRECT);
	if (bnum == 0) {
		return ERROR;
	}

	return bnum;
}

int AllocateBlockInInode(struct inode* inode, int inum) {
	int i;
	for (i = 0; i < NUM_DIRECT; ++i) {
		if (inode->direct[i] == 0) {
			int bnum = FindFreeBlock();
			if (bnum == ERROR) {
				return ERROR;
			}

			inode->direct[i] = bnum;
			SetDirty(inode_cache, inum);
			return 0;
		}
	}

	if (inode->indirect == 0) {
		int bnum = FindFreeBlock();
		if (bnum == ERROR) {
			return ERROR;
		}

		void* block = GetBlockByBnum(bnum);
		if (block == NULL) {
			return ERROR;
		}

		memset(block, 0, BLOCKSIZE);
		inode->indirect = bnum;
		SetDirty(inode_cache, inum);
	}

	void* indirect_block = GetBlockByBnum(inode->indirect);
	if (indirect_block == NULL) {
		return ERROR;
	}

	for (i = 0; i < BLOCKSIZE / sizeof(int); ++i) {
		if (((int*)indirect_block)[i] == 0) {
			int bnum = FindFreeBlock();
			if (bnum == ERROR) {
				return ERROR;
			}

			((int*)indirect_block)[i] = bnum;
			SetDirty(block_cache, inode->indirect);
			return 0;
		}
	}

	return ERROR;
}

int CountDirEntry(struct inode* dir_inode, int dir_inum) {
	if (dir_inode == NULL || dir_inode->type != INODE_DIRECTORY) {
		return ERROR;
	}

	int count = 0;
	int i;
	for (i = 0; i < dir_inode->size / sizeof(struct dir_entry); ++i) {
		int block_index = i * sizeof(struct dir_entry) / BLOCKSIZE;
		if (block_index >= NUM_DIRECT && dir_inode->indirect == 0) {
			return ERROR;
		}

		int bnum;
		if (block_index < NUM_DIRECT) {
			bnum = dir_inode->direct[block_index];
		} else {
			bnum = GetBnumFromIndirectBlock(dir_inode->indirect, block_index - NUM_DIRECT);
		}

		void* block = GetBlockByBnum(bnum);
		if (block == NULL) {
			return ERROR;
		}

		struct dir_entry entry = ((struct dir_entry*)block)[i % DIR_ENTRY_PER_BLOCK];
		if (entry.inum != 0) {
			count++;
		}
	}
	return count;
}

int DeleteDirEntry(struct inode* dir_inode, int dir_inum, int inum) {
	if (dir_inode == NULL || dir_inode->type != INODE_DIRECTORY) {
		return ERROR;
	}

	if (inum < 1 || inum > header.num_inodes) {
		return ERROR;
	}

	int i;
	for (i = 0; i < dir_inode->size / sizeof(struct dir_entry); ++i) {
		int block_index = i * sizeof(struct dir_entry) / BLOCKSIZE;
		if (block_index >= NUM_DIRECT && dir_inode->indirect == 0) {
			return ERROR;
		}

		int bnum;
		if (block_index < NUM_DIRECT) {
			bnum = dir_inode->direct[block_index];
		} else {
			bnum = GetBnumFromIndirectBlock(dir_inode->indirect, block_index - NUM_DIRECT);
		}

		void* block = GetBlockByBnum(bnum);
		if (block == NULL) {
			return ERROR;
		}

		struct dir_entry entry = ((struct dir_entry*)block)[i % DIR_ENTRY_PER_BLOCK];
		if (entry.inum == inum) {
			entry.inum = 0;
			SetDirty(block_cache, bnum);
			return 0;
		}
	}
	return ERROR;
}

int CreateDirEntry(struct inode* dir_inode, int dir_inum, int inum, char* name) {
	if (dir_inode == NULL || dir_inode->type != INODE_DIRECTORY) {
		return ERROR;
	}

	if (inum < 1 || inum > header.num_inodes) {
		return ERROR;
	}

	if (name == NULL || strlen(name) > DIRNAMELEN) {
		return ERROR;
	}

	int len = strlen(name);
	if (DIRNAMELEN < len) {
		len = DIRNAMELEN;
	}

	/* Find empty dir entry within the current size */
	int i;
	for (i = 0; i < dir_inode->size / sizeof(struct dir_entry); ++i) {
		int block_index = i * sizeof(struct dir_entry) / BLOCKSIZE;
		if (block_index >= NUM_DIRECT && dir_inode->indirect == 0) {
			return ERROR;
		}

		int bnum;
		if (block_index < NUM_DIRECT) {
			bnum = dir_inode->direct[block_index];
		} else {
			bnum = GetBnumFromIndirectBlock(dir_inode->indirect, block_index - NUM_DIRECT);
		}

		void* block = GetBlockByBnum(bnum);
		if (block == NULL) {
			return ERROR;
		}

		struct dir_entry entry = ((struct dir_entry*)block)[i % DIR_ENTRY_PER_BLOCK];
		if (entry.inum == 0) {
			entry.inum = inum;
			memcpy(entry.name, name, len);
			SetDirty(block_cache, bnum);
			return 0;
		}
	}

	/* Allocate new block */
	if (dir_inode->size % BLOCKSIZE == 0) {
		AllocateBlockInInode(dir_inode, dir_inum);
	}

	/* Setup a dir entry out of the current size */
	dir_inode->size += sizeof(struct dir_entry);
	int bnum;
	int block_index = i * sizeof(struct dir_entry) / BLOCKSIZE;
	if (block_index < NUM_DIRECT) {
		bnum = dir_inode->direct[block_index];
	} else {
		bnum = GetBnumFromIndirectBlock(dir_inode->indirect, block_index - NUM_DIRECT);
	}

	void* block = GetBlockByBnum(bnum);
	if (block == NULL) {
		return ERROR;
	}

	struct dir_entry entry = ((struct dir_entry*)block)[i % DIR_ENTRY_PER_BLOCK];
	entry.inum = inum;
	memcpy(entry.name, name, len);
	SetDirty(block_cache, bnum);
	return 0;
}

/* Return the starting position of file name in path */
int GetFileNameIndex(char* pathname) {
	int i;
    int filename_index = 0;

    if (pathname[strlen(pathname) - 1] == '/') {
    	return ERROR;
    }

    for (i = strlen(pathname) - 2; i >= 0; --i) {
		if (pathname[i] == '/') {
			filename_index = i + 1;
			break;
		}
    }

    return filename_index;
}

int FindFreeBlock(void) {
	if (num_free_blocks == 0) {
		return ERROR;
	}

	int i;
    for (i = 1; i <= header.num_blocks; ++i) {
        if (free_blocks[i]) {
        	--num_free_blocks;
            return i;
        }
    }

    return ERROR;
}

void RecycleFreeBlock(int bnum) {
	if (bnum < 1 || bnum > header.num_blocks) {
		return;
	}

	free_blocks[bnum] = false;
	++num_free_blocks;
}

int FindFreeInode(void) {
	if (num_free_inodes == 0) {
		return ERROR;
	}

	int i;
    for (i = 1; i <= header.num_inodes; ++i) {
        if (free_inodes[i]) {
        	--num_free_inodes;
            return i;
        }
    }

    return ERROR;
}

void RecycleFreeInode(int inum) {
	if (inum < 1 || inum > header.num_inodes) {
		return;
	}

	free_inodes[inum] = false;
	++num_free_inodes;
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


void SyncInodeCache() {
	CacheNode* current = inode_cache->head;

	while (current != NULL) {
		if (current->dirty) {
			current->dirty = false;
			void* block = GetBlockByInum(current->key);
			int bnum = GetBlockNumFromInodeNum(current->key);
			if (block == NULL) {
				return;
			}

		    /* Save inode in the block and set dirty bit for that block */
		    int offset = current->key % INODE_PER_BLOCK;
		    memcpy((struct inode*)block + offset, (struct inode*)(current->value), sizeof(struct inode));
		    SetDirty(block_cache, bnum);
		}

		current = current->next;
	}
}

void SyncBlockCache() {
	CacheNode* current = block_cache->head;

	while (current != NULL) {
		if (current->dirty) {
			current->dirty = false;

			if (WriteSector(current->key, current->value) == ERROR) {
		        printf("Write Sector #%d failed\n", current->key);
		    }
		}

		current = current->next;
	}
}

int RecycleBlocksInInode(int inum) {
	struct inode* inode = GetInodeByInum(inum);
    if (inode == NULL) {
        return ERROR;
    }

    int i;
    for (i = 0; i < NUM_DIRECT; ++i) {
        if (inode->direct[i] == 0) {
            break;
        }

        RecycleFreeBlock(inode->direct[i]);
        inode->direct[i] = 0;
    }

    if (inode->indirect > 0) {
    	int j;
    	for (j = 0; j < BLOCKSIZE / sizeof(int); ++j) {
    		int bnum = GetBnumFromIndirectBlock(inode->indirect, j);
    		if (bnum == 0) {
    			break;
    		}

    		RecycleFreeBlock(bnum);
    	}
    	RecycleFreeBlock(inode->indirect);
    	inode->indirect = 0;
    }

    inode->size = 0;
    SetDirty(inode_cache, inum);
    return 0;
}