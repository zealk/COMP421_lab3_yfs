/*
 *  External definitions for the Yalnix File System user interface
 */

#ifndef _iolib_h
#define _iolib_h

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  The values used for 'whence' on a Seek call:
 */
#define	SEEK_SET	0	/* Set position to 'offset' */
#define	SEEK_CUR	1	/* Set position to current plus 'offset' */
#define	SEEK_END	2	/* Set position to EOF plus 'offset' */

/*
 *  The structure used to return information on a Stat call:
 */
struct Stat {
    int inum;		/* inode number of file */
    int type;		/* type of file (e.g., INODE_REGULAR) */
    int size;		/* size of file in bytes */
    int nlink;		/* link count of file */
};

/*
 *  Function prototypes for YFS calls:
 */
extern int Open(char *);
extern int Close(int);
extern int Create(char *);
extern int Read(int, void *, int);
extern int Write(int, void *, int);
extern int Seek(int, int, int);
extern int Link(char *, char *);
extern int Unlink(char *);
extern int SymLink(char *, char *);
extern int ReadLink(char *, char *, int);
extern int MkDir(char *);
extern int RmDir(char *);
extern int ChDir(char *);
extern int Stat(char *, struct Stat *);
extern int Sync(void);
extern int Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* _iolib_h */
