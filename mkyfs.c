/*
 *  Create an empty Yalnix file system on the Yalnix DISK.
 *
 *  This is a Unix program (not a Yalnix program).  It creates a Yalnix
 *  file system in the Unix file named "DISK".  This file is then used
 *  by the Yalnix hardware simulation as the Yalnix disk device.  You
 *  can write other Unix programs or use existing Unix tools to create
 *  "interesting" DISK contents to test your server on or to examine
 *  the "raw" contents of your file system on disk.  For example,
 *  running "od -X DISK" or "od -c DISK" under Unix can be useful ways
 *  to get a quick look at the DISK contents.
 *
 *  Usage: mkyfs [num_inodes]
 *
 *  The default number of inodes if num_inodes is not specified is
 *  given by the DEFAULT_NUM_INODES constant below.
 *
 *  RUN THIS COMMAND AS A UNIX PROGRAM, NOT AS A YALNIX PROGRAM.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <comp421/filesystem.h>

#define	INODES_PER_BLOCK	(BLOCKSIZE/INODESIZE)

#define DISK_FILE_NAME		"DISK"
#define DEFAULT_NUM_INODES	(6 * INODES_PER_BLOCK - 1)

union {
    struct fs_header hdr;
    struct inode inode[INODES_PER_BLOCK];
    char buf[BLOCKSIZE];
} block;

main(int argc, char **argv)
{
    int disk;
    int num_inodes = DEFAULT_NUM_INODES;
    int i;
    struct inode *inodes;
    int inodes_size;
    struct dir_entry root[2];

    if (argc > 1) {
	if (sscanf(argv[1], "%d", &num_inodes) != 1) {
	    fprintf(stderr, "usage: mkyfs [num_inodes]\n");
	    exit(1);
	}
    }

    if ((disk = creat(DISK_FILE_NAME, 0666)) < 0) {
	perror(DISK_FILE_NAME);
	exit(1);
    }

    lseek(disk, BLOCKSIZE, 0);

    inodes_size = (num_inodes + 1) * INODESIZE;
    /* force rounded up to BLOCKSIZE multiple */
    inodes_size = (inodes_size + BLOCKSIZE - 1) & ~(BLOCKSIZE - 1);
    inodes = (struct inode *)malloc(inodes_size);

    ((struct fs_header *)inodes)->num_blocks = NUMSECTORS;
    ((struct fs_header *)inodes)->num_inodes = num_inodes;

    inodes[1].type = INODE_DIRECTORY;
    inodes[1].nlink = 2;
    inodes[1].size = 2 * sizeof(struct dir_entry);
    inodes[1].direct[0] = (inodes_size / BLOCKSIZE) + 1;

    for (i = 2; i <= num_inodes; i++) {
	inodes[i].type = INODE_FREE;
    }

    if (write(disk, inodes, inodes_size) != inodes_size) {
	perror("write inodes");
	unlink(DISK_FILE_NAME);
	exit(1);
    }

    memset((void *)root, '\0', sizeof(root));
    root[0].inum = ROOTINODE;
    root[0].name[0] = '.';
    root[1].inum = ROOTINODE;
    root[1].name[0] = '.';
    root[1].name[1] = '.';

    if (write(disk, root, sizeof(root)) != sizeof(root)) {
	perror("write root");
	unlink(DISK_FILE_NAME);
	exit(1);
    }

    /*
     *  Seek to the last block of the DISK and write it full of zeros.
     *  In Unix, this leaves a "hole" in the file, which will act
     *  exactly as if it had been written full of zeros.  When reading
     *  from this space skipped over here, Unix will read it as all
     *  zeros.  By making this a hole, it is faster to run mkyfs,
     *  and it saves real Unix disk space since a hole doesn't consume
     *  physical disk blocks.
     */
    lseek(disk, BLOCKSIZE * (NUMSECTORS - 1), 0);
    memset((void *)&block, '\0', BLOCKSIZE);
    if (write(disk, &block, BLOCKSIZE) != BLOCKSIZE) {
	perror("write last zero");
	unlink(DISK_FILE_NAME);
	exit(1);
    }

    exit(0);
}
