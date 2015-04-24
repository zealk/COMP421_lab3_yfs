#include "../include/yfs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <comp421/yalnix.h>
#include "../include/fscache.h"

void YfsOpen(Message* msg, int pid) {
    printf("Executing YfsOpen()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, pathname, msg->addr1, MAXPATHNAMELEN) == ERROR) {
        printf("CopyFrom() error\n");
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    /* Parse the file name */
    int inum = ParsePathName(msg->data1, pathname);
    if (inum == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
    	return;
    }
   
    struct inode* inode = GetInodeByInum(inum);
    if (inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

	msg->data1 = inum;
	Reply((void*)msg, pid);
}

void YfsCreate(Message* msg, int pid) {
    printf("Executing YfsCreate()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, pathname, msg->addr1, MAXPATHNAMELEN) == ERROR) {
        printf("CopyFrom() error\n");
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    /* Check if all directores is valid */
    int dir_inum = ParsePathDir(msg->data1, pathname);
    if (dir_inum == ERROR) {
        printf("Invalid directory\n");
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int filename_index = GetFileNameIndex(pathname);
    char filename[strlen(pathname) - filename_index + 1];
    memcpy(filename, pathname + filename_index, strlen(pathname) - filename_index);
    filename[strlen(pathname) - filename_index] = '\0';
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        printf("Invalid file name\n");
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    struct inode* dir_inode = GetInodeByInum(dir_inum);
    int inum = GetInumByComponentName(dir_inode, filename);

    /* If file name cannot be found */
    if (inum == 0) {
        inum = FindFreeInode();
        if (inum == ERROR) {
            msg->type = ERROR;
            Reply((void*)msg, pid);
            return;
        }

        struct inode* inode = GetInodeByInum(inum);
        if (inode == NULL) {
            msg->type = ERROR;
            Reply((void*)msg, pid);
            return;
        }

        inode->type = INODE_REGULAR;
        inode->nlink = 1;
        ++inode->reuse;
        inode->size = 0;
        memset(inode->direct, 0, NUM_DIRECT * sizeof(int));
        inode->indirect = 0;

        if (CreateDirEntry(dir_inode, dir_inum, inum, filename) == ERROR) {
            printf("Can't create new dir entry\n");
            msg->type = ERROR;
            Reply((void*)msg, pid);
            return;
        }

        SetDirty(inode_cache, inum);
    /* If file name can be found */
    } else {
        printf("File has existed. Set file size to 0\n");
        if (RecycleBlocksInInode(inum) == ERROR) {
            msg->type = ERROR;
            Reply((void*)msg, pid);
            return;
        }
    }

    msg->data1 = inum;
    Reply((void*)msg, pid);
}

void YfsRead(Message* msg, int pid) {
    printf("Executing YfsRead()\n");
    struct inode* inode = GetInodeByInum(msg->data1);
    if (inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int size = msg->data2;
    int seek_pos = msg->data3;
    if (seek_pos + size > inode->size) {
        size = inode->size - seek_pos;
    }

    int len = 0;
    char* buf = (char*)malloc(size);
    while (size > 0) {
        int bnum = GetBnumBySeekPosition(inode, seek_pos);
        if (bnum == ERROR) {
            free(buf);
            msg->type = ERROR;
            Reply((void*)msg, pid);
            return;
        }

        char* block = (char*)GetBlockByBnum(bnum);
        int remaining_len = BLOCKSIZE - (seek_pos % BLOCKSIZE);
        if (remaining_len < size) {
            memcpy(block, block + seek_pos % BLOCKSIZE, remaining_len);
            len += remaining_len;
            seek_pos += remaining_len;
            size -= remaining_len;
            continue;
        }

        memcpy(block, block + seek_pos % BLOCKSIZE, size);
        len += size;
        seek_pos += size;
        size = 0;
    }

    if (CopyTo(pid, msg->addr1, (void*)buf, len) == ERROR) {
        free(buf);
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    free(buf);
    msg->type = len;
    Reply((void*)msg, pid);
}

void YfsWrite(Message* msg, int pid) {
    printf("Executing YfsWrite()\n");
    struct inode* inode = GetInodeByInum(msg->data1);
    if (inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    /* Check inode's type */
    if (inode->type == INODE_DIRECTORY) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int size = msg->data2;
    int seek_pos = msg->data3;
    char* buf = (char*)malloc(size);
    if (CopyFrom(pid, (void*)buf, msg->addr1, size) == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int len = 0;
    while (size > 0) {
        int bnum = GetBnumBySeekPosition(inode, seek_pos);
        if (bnum == ERROR) {
            bnum = AllocateBlockInInode(inode, msg->data1);
            if (bnum == ERROR) {
                msg->type = ERROR;
                Reply((void*)msg, pid);
                return;
            }
        }

        char* block = (char*)GetBlockByBnum(bnum);
        int remaining_len = BLOCKSIZE - (seek_pos % BLOCKSIZE);
        if (remaining_len < size) {
            memcpy(block + seek_pos % BLOCKSIZE, buf + len, remaining_len);
            len += remaining_len;
            seek_pos += remaining_len;
            size -= remaining_len;
            SetDirty(block_cache, bnum);
            continue;
        }

        memcpy(block + seek_pos % BLOCKSIZE, buf + len, size);
        len += size;
        seek_pos += size;
        size = 0;
        SetDirty(block_cache, bnum);
    }

    free(buf);
    if (seek_pos > inode->size) {
        inode->size = seek_pos;
        SetDirty(inode_cache, msg->data1);
    }

    msg->type = len;
    Reply((void*)msg, pid);
}

void YfsSeek(Message* msg, int pid) {
    printf("Executing YfsSeek()\n");

    /* Get newest file size */
    struct inode* inode = GetInodeByInum(msg->data1);
    if (inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }
    
    int* pos_addr = NULL;
    if (CopyFrom(pid, (void*)(pos_addr), msg->addr1, sizeof(int)) == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int whence = msg->data3;
    switch (whence) {
        case SEEK_SET:
            whence = 0;
            break;
        case SEEK_CUR:
            whence = *pos_addr;
            break;
        case SEEK_END:
            whence = inode->size;
            break;
    }

    int seek_pos = whence + msg->data2;
    if (seek_pos < 0 || seek_pos > inode->size) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    msg->type = seek_pos;
    Reply((void*)msg, pid);
}

void YfsLink(Message* msg, int pid) {
    printf("Executing YfsLink()\n");
    char oldname[MAXPATHNAMELEN];
    char newname[MAXPATHNAMELEN];

    if (CopyFrom(pid, (void*)oldname, msg->addr1, MAXPATHNAMELEN) == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }
    
    if (CopyFrom(pid, (void*)newname, msg->addr2, MAXPATHNAMELEN) == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }
    
    /* Get old file's directory */
    int old_dir_inum = ParsePathDir(msg->data1, oldname);
    if (old_dir_inum == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    /* Get inode of old file's directory */
    struct inode* dir_inode = GetInodeByInum(old_dir_inum);
    if (dir_inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    /* Get inode of old file */
    int filename_index = GetFileNameIndex(oldname);
    char filename[strlen(oldname) - filename_index + 1];
    memcpy(filename, oldname + filename_index, strlen(oldname) - filename_index);
    filename[strlen(oldname) - filename_index] = '\0';
    int old_inum = GetInumByComponentName(dir_inode, filename);
    struct inode* old = GetInodeByInum(old_inum);
    int new_inum = ParsePathName(msg->data1, newname);

    if (old == NULL || new_inum != ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    if (old->type == INODE_DIRECTORY) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int new_dir_inum = ParsePathDir(msg->data1, newname);
    if (new_dir_inum == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    struct inode* new_dir_inode = GetInodeByInum(new_dir_inum);
    if (new_dir_inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    if (CreateDirEntry(new_dir_inode, new_dir_inum, old_inum, filename) == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    ++old->nlink;
    SetDirty(inode_cache, old_inum);
    Reply((void*)msg, pid);
}

void YfsUnlink(Message* msg, int pid) {
    printf("Executing YfsUnlink()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, (void*)pathname, msg->addr1, MAXPATHNAMELEN) == ERROR)
        goto ERR;

    /* Get pathname's directory */
    int file_dir_inum = ParsePathDir(msg->data1, pathname);
    if (file_dir_inum == ERROR)
        goto ERR;

    /* Get inode of pathname's directory */
    struct inode* dir_inode = GetInodeByInum(file_dir_inum);
    if (dir_inode == NULL)
        goto ERR;

    /* Get filename from pathname */
    int filename_index = GetFileNameIndex(pathname);
    char filename[strlen(pathname) - filename_index + 1];
    memcpy(filename, pathname + filename_index, strlen(pathname) - filename_index);
    filename[strlen(pathname) - filename_index] = '\0';

    /* Get file inode */
    int file_inum = GetInumByComponentName(dir_inode, filename);
    struct inode* file_inode = GetInodeByInum(file_inum);

    if (file_inode == NULL)
        goto ERR;
    if (file_inode->type == INODE_DIRECTORY)
        goto ERR;
    if (DeleteDirEntry(dir_inode, file_dir_inum, file_inum, filename) == ERROR)
        goto ERR;

    if (!(--file_inode->nlink)) {
        RecycleBlocksInInode(file_inum);
        RecycleFreeInode(file_inum);
    }

    SetDirty(inode_cache, file_inum);
    Reply((void*)msg, pid);
    return;

    ERR:
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
}

void YfsSymLink(Message* msg, int pid) {
    printf("Executing YfsSymLink()\n");
    char oldname[MAXPATHNAMELEN];
    char newname[MAXPATHNAMELEN];

    if (CopyFrom(pid, (void*)oldname, msg->addr1, MAXPATHNAMELEN) == ERROR)
        goto ERR;   
    if (CopyFrom(pid, (void*)newname, msg->addr2, MAXPATHNAMELEN) == ERROR)
        goto ERR;
    /* Null pathname */
    if (oldname[0] == '\0')
        goto ERR;
    /* New name existed */
    int new_inum = ParsePathName(msg->data1, newname);
    if (new_inum != Error)
        goto ERR;
    /* Create new symbol link file */
    /* Check if all directores is valid */
    int dir_inum = ParsePathDir(msg->data1, newname);
    if (dir_inum == ERROR)
        goto ERR;

    /* Get inode of pathname's directory */
    struct inode* dir_inode = GetInodeByInum(file_dir_inum);
    if (dir_inode == NULL)
        goto ERR;

    /* Check if newname is valid */
    int filename_index = GetFileNameIndex(newname);
    char filename[strlen(newname) - filename_index + 1];
    memcpy(filename, newname + filename_index, strlen(newname) - filename_index);
    filename[strlen(newname) - filename_index] = '\0';
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
        goto ERR;
    /* Check if newname exists again */
    inum = GetInumByComponentName(dir_inode, filename);
    if (inum)
        goto ERR;

    inum = FindFreeInode();
    if (inum == ERROR)
        goto ERR;

    struct new_inode* inode = GetInodeByInum(inum);
    if (inode == NULL)
            goto ERR;

    /* Initialize new inode of symbolic link */
    inode->type = INODE_SYMLINK;
    inode->nlink = 1;
    ++inode->reuse;
    inode->size = 0;
    memset(inode->direct, 0, NUM_DIRECT * sizeof(int));
    inode->indirect = 0;
    SetDirty(inode_cache,inum);

    /* Create directory entry for new inode */
    if (CreateDirEntry(dir_inode, dir_inum, inum, filename) == ERROR)
        goto ERR;
    
    /* Allocate a new block to store oldname */
    int bnum = AllocateBlockInInode(inode,inum);
    if (bnum == ERROR)
        goto ERR;
    void* block = GetBlockByBnum(bnum);
    if (block == NULL)
        goto ERR;
    /* Path name should be stored in one block */
    if (BLOCKSIZE < MAXPATHNAMELEN)
        goto ERR;
    
    memcpy(block, oldname, sizeof(oldname));
    inode->size = sizeof(oldname);

    SetDirty(block_cache,bnum);
    
    Reply((void*)msg, pid);
    return;

    ERR:
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
}

void YfsReadLink(Message* msg, int pid) {
    printf("Executing YfsReadLink()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, (void*)pathname, msg->addr1, MAXPATHNAMELEN) == ERROR)
        goto ERR;

    int maxLen = msg->data2;

    /* Get pathname's directory */
    int file_dir_inum = ParsePathDir(msg->data1, pathname);
    if (file_dir_inum == ERROR)
        goto ERR;

    /* Get inode of pathname's directory */
    struct inode* dir_inode = GetInodeByInum(file_dir_inum);
    if (dir_inode == NULL)
        goto ERR;

    /* Get filename from pathname */
    int filename_index = GetFileNameIndex(pathname);
    char filename[strlen(pathname) - filename_index + 1];
    memcpy(filename, pathname + filename_index, strlen(pathname) - filename_index);
    filename[strlen(pathname) - filename_index] = '\0';

    /* Get file inode */
    int file_inum = GetInumByComponentName(dir_inode, filename);
    struct inode* file_inode = GetInodeByInum(file_inum);

    if (file_inode == NULL)
        goto ERR;
    if (file_inode->type != INODE_SYMLINK)
        goto ERR;

    /* Oldname should be stored in first block */
    int bnum = file_inode->direct[0];
    if (bnum == 0)
        goto ERR;
    void* block = GetBlockByBnum(bnum);
    if (block == NULL)
        goto ERR;

    int actualLen = strlen((char *)block);
    /* Oldname length == MAXPATHNAMELEN */
    if (actualLen > MAXPATHNAMELEN)
        actualLen = MAXPATHNAMELEN;
    /* Truncate */
    if (actualLen < maxLen)
        maxLen = actualLen;

    if (CopyTo(pid, msg->addr2, block, maxLen) == ERROR)
        goto ERR;

    Reply((void*)msg, pid);
    return;

    ERR:
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
}

void YfsMkDir(Message* msg, int pid) {
    printf("Executing YfsMkDir()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, (void*)pathname, msg->addr1, MAXPATHNAMELEN) == ERROR)
        goto ERR;
    /* new name exists */
    int new_inum = ParsePathName(msg->data1, pathname);
    if (new_inum != Error)
        goto ERR;

    /* Check if all directores is valid */
    int dir_inum = ParsePathDir(msg->data1, pathname);
    if (dir_inum == ERROR)
        goto ERR;

    /* Get inode of pathname's directory */
    struct inode* dir_inode = GetInodeByInum(dir_inum);
    if (dir_inode == NULL)
        goto ERR;

    /* Check if pathname is valid */
    int filename_index = GetFileNameIndex(pathname);
    char filename[strlen(pathname) - filename_index + 1];
    memcpy(filename, pathname + filename_index, strlen(pathname) - filename_index);
    filename[strlen(pathname) - filename_index] = '\0';
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
        goto ERR;
    /* Check if pathname exists again */
    inum = GetInumByComponentName(dir_inode, filename);
    if (inum)
        goto ERR;

    inum = FindFreeInode();
    if (inum == ERROR)
        goto ERR;

    struct new_inode* inode = GetInodeByInum(inum);
    if (inode == NULL)
        goto ERR;

    /* Initialize new directory node */
    inode->type = INODE_DIRECTORY;
    inode->nlink = 1;
    ++inode->reuse;
    inode->size = 0;
    memset(inode->direct, 0, NUM_DIRECT * sizeof(int));
    inode->indirect = 0;
    SetDirty(inode_cache,inum);

    char current[3] = ".";
    char parent[3] = "..";

    /* Parent -> New one */
    if (CreateDirEntry(dir_inode, dir_inum, inum, filename) == ERROR)
        goto ERR;
    /* New one -> . */
    if (CreateDirEntry(inode, inum, inum, current) == ERROR)
        goto ERR;
    /* New one -> .. */
    if (CreateDirEntry(inode, inum, dir_inum, parent) == ERROR)
        goto ERR;

    Reply((void*)msg, pid);
    return;

    ERR:
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;    
}

void YfsRmDir(Message* msg, int pid) {
    printf("Executing YfsRmDir()\n");

}

void YfsChDir(Message* msg, int pid) {
    printf("Executing YfsChDir()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, (void*)pathname, msg->addr1, MAXPATHNAMELEN) == ERROR) {
        printf("CopyFrom() error\n");
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int inum = ParsePathName(msg->data1, pathname);
    if (inum == ERROR) {
        printf("ParsePathName() error\n");
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    struct inode* inode = GetInodeByInum(inum);
    if (inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    if (inode->type != INODE_DIRECTORY) {
        printf("The path %s is not a directory\n", pathname);
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    msg->data1 = inum;
    Reply((void*)msg, pid);
}

void YfsStat(Message* msg, int pid) {
    printf("Executing YfsStat()\n");
    char pathname[MAXPATHNAMELEN];
    if (CopyFrom(pid, (void*)pathname, msg->addr1, MAXPATHNAMELEN) == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int dir_inum = ParsePathDir(msg->data1, pathname);
    if (dir_inum == ERROR) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    struct inode* dir_inode = GetInodeByInum(dir_inum);
    if (dir_inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    int filename_index = GetFileNameIndex(pathname);
    char filename[strlen(pathname) - filename_index + 1];
    memcpy(filename, pathname + filename_index, strlen(pathname) - filename_index);
    filename[strlen(pathname) - filename_index] = '\0';
    int inum = GetInumByComponentName(dir_inode, filename);
    if (inum == ERROR || inum == 0) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    struct inode* inode = GetInodeByInum(inum);
    if (inode == NULL) {
        msg->type = ERROR;
        Reply((void*)msg, pid);
        return;
    }

    msg->type = inum;
    msg->data1 = inode->type;
    msg->data2 = inode->size;
    msg->data3 = inode->nlink;
    Reply((void*)msg, pid);
}

void YfsSync(Message* msg, int pid) {
    printf("Executing YfsSync()\n");
    SyncInodeCache();
    SyncBlockCache();
    Reply(msg, pid);
}

void YfsShutDown(Message* msg, int pid) {
    printf("Executing YfsShutDown()\n");
    SyncInodeCache();
    SyncBlockCache();
    Reply(msg, pid);
    printf("Yalnix File System is shuting down ...\n");
    Exit(0);
}
