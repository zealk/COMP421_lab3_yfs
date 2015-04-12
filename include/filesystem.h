/*
 *  The format of the file system and other constants used by YFS
 */

#ifndef _filesystem_h
#define _filesystem_h

#include <sys/types.h>
#include <comp421/hardware.h>

/*
 *  The size of blocks in YFS is the same as the disk hardware sector size.
 *  SECTORSIZE and NUMSECTORS are defined in hardware.h.
 */
#define BLOCKSIZE SECTORSIZE	/* Each file system block is 1 sector */
#define NUMBLOCKS NUMSECTORS	/* Same number of blocks as sectors */

/*
 *  The boot block is always block number 0 on the disk.  This block
 *  is not used by the file system.
 *
 *  After the boot block, a number of following blocks are filled with
 *  inodes.  However, instead of the first inode in the first of these
 *  blocks, the file system header structure is stored.  The file
 *  system header defines the size of the other parts of the file
 *  system.
 */

struct fs_header {
    int num_blocks;		/* total blocks in file system */
    int num_inodes;		/* number of inodes in file system */
    int padding[14];		/* make fs_header and inode same size */
};

/*
 *  The inodes follow the file system header and are contiguous in
 *  the following blocks.  The size of the file system header is the
 *  same as that of each inode.  This size is an integer fraction of
 *  the file system block size, and thus an integer number of inodes
 *  pack nicely into each block.
 */

#define	INODESIZE	64	/* The size in bytes of each inode */
#define NUM_DIRECT	13	/* number of direct block #s in inode */

struct inode {
    short type;			/* file type (e.g., directory or regular) */
    short nlink;		/* number of hard links to inode */
    int   size;			/* file size in bytes */
    int   direct[NUM_DIRECT];	/* block numbers for 1st 13 blocks */
    int   indirect;		/* block number of indirect block */
};

/*
 *  The 'type' field in each inode indicates the inode's (and file's) type:
 */
#define	INODE_FREE	0	/* This inode is not used for any file */
#define	INODE_DIRECTORY	1	/* This inode describes a directory */
#define	INODE_REGULAR	2	/* This inode describes a regular file */
#define	INODE_SYMLINK	3	/* This inode describes a symbolic link */

/*
 *  The Yalnix file system implements a tree-structured directory
 *  through the use of the INODE_DIRECTORY inode type described above.
 *
 *  The root of this tree-structured directory is always ROOTINODE.
 */

#define	ROOTINODE	1	/* The inode # of the root directory */

/*
 *  Within each directory, the directory entries are represented by
 *  a contiguous sequence of directory entries, stored within the data
 *  bytes of the directory file.  All directory entries have the same
 *  format and are a constant size.
 *
 *  The 'name' field in a dir_entry is *not* terminated by a null byte.
 *  You cannot use functions like strcpy and strcmp to access it.  You
 *  can, however, use memcpy and memcmp, etc., to access it, if you
 *  want to.
 */

#define	DIRNAMELEN	30	/* length in bytes of name in dir_entry */

struct dir_entry {
    short inum;			/* inode number */
    char  name[DIRNAMELEN];	/* file name (not null-terminated) */
};

/*
 *  A pathname is the character string argument (null-terminated as a
 *  normal C-language character string) that is passed by users on 
 *  file system calls, such as Open, Unlink, Link, MkDir, etc.  For
 *  example
 *
 *     /abc/def/ghi.c
 *     xyz/123-456
 *     ../../foo/bar/baz/example.txt
 *     silly/./././././././example.txt
 *
 *  would all be valid pathnames (depending on whether the named files
 *  and directories actually exist or not).
 */

/*
 *  The maximum length of the entire pathname, including the null character,
 *  is limited to MAXPATHNAMELEN characters.  This limit of MAXPATHNAMELEN
 *  characters applies only to the length of the pathname string when
 *  presented as an argument to a Yalnix file system call.  The fact of
 *  whether this pathname is an absolute pathname or a relative pathname,
 *  or the possible presence of symbolic links encountered while processing
 *  this pathname, do not count against that limit of MAXPATHNAMELEN
 *  characters.  The limit of MAXPATHNAMELEN characters literally
 *  applies only to the argument of the call itself.
 */
#define	MAXPATHNAMELEN	256	/* maximum length in bytes of a pathname */

/*
 *  The maximum number of symbolic link traversals that may be performed
 *  during processing of a single pathname that was the argument to
 *  any Yalnix file system operation is limited to MAXSYMLINKS symbolic
 *  link traversals.  If, in processing any single pathname that was an
 *  argument to any Yalnix file system operation, you would need to
 *  traverse more than MAXSYMLINKS symbolnic links, you should terminate
 *  processing of that pathname and instead return ERROR from your
 *  file server.
 */
#define	MAXSYMLINKS	20	/* maximum symlink traversals in a pathname */

/*
 *  The file server maintains a cache of disk blocks and a cache of
 *  inodes.  These caches have a *constant* size:
 */
#define	BLOCK_CACHESIZE	32	/* number of disk blocks cached */
#define	INODE_CACHESIZE	16	/* number of inodes cached */

/*
 *  For simplicity, the YFS I/O library used by client processes
 *  is limited to a defined maximum number of files for the process.
 *  The YFS server must support an *arbitrary* number of open files,
 *  since it may be serving many separate processes.
 */
#define	MAX_OPEN_FILES	16	/* max # of open files per process */

/*
 *  Client processes Send interact with the YFS file server through
 *  Yalnix IPC calls.  To Send a message to the YFS server, use
 *
 *    Send(msg, -FILE_SERVER)
 *
 *  where 'msg' is some message structure that you define for
 *  interaction between your YFS library and your YFS server.  The
 *  "msg' should be the address of a C 'struct' that has a size
 *  of 32 bytes, since all messages in Yalnix IPC are of size 32.
 *  You must define your own message structure for communication 
 *  between your library and your server.  Your server must use
 *  the Yalnix Register kernel call to register itself as service_id
 *  FILE_SERVER.
 *
 *  In addition to the basic Send/Receive/Reply-type calls, you will
 *  also need to use CopyFrom and CopyTo in your server to copy larger
 *  chuncks of data from/to a client program that has sent a message
 *  to the server requesting some operation.  For example, you
 *  would use one or more CopyFrom calls to get the data on a Write
 *  call or to return the data on a Read call.  You would also use
 *  CopyFrom to read a pathname from the client process, since a
 *  pathname in general could be much longer than will fit in a 
 *  constant-sized 32-byte message structure.
 */

#define	FILE_SERVER	1	/* The Yalnix service_id used by YFS */

#endif /* _filesystem_h */
