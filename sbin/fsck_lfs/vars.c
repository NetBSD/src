/* $NetBSD: vars.c,v 1.4 2000/06/14 18:44:01 perseant Exp $	 */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>		/* XXX */
#include <ufs/lfs/lfs.h>
#include "fsck.h"

/* variables previously of file scope (from fsck.h) */
struct bufarea  bufhead;	/* head of list of other blks in filesys */
struct bufarea  sblk;		/* file system superblock */
struct bufarea  iblk;		/* ifile on-disk inode block */
struct bufarea *pdirbp;		/* current directory contents */
struct bufarea *pbp;		/* current inode block */
struct bufarea *getdatablk(daddr_t, long);
int             iinooff;	/* ifile inode offset in block of inodes */

struct dups    *duplist;	/* head of dup list */
struct dups    *muldup;		/* end of unique duplicate dup block numbers */

struct zlncnt  *zlnhead;	/* head of zero link count list */

daddr_t		idaddr;		/* inode block containing ifile inode */
long            numdirs, listmax, inplast;

long            dev_bsize;	/* computed value of DEV_BSIZE */
long            secsize;	/* actual disk sector size */
char            nflag;		/* assume a no response */
char            yflag;		/* assume a yes response */
int             bflag;		/* location of alternate super block */
int             debug;		/* output debugging info */
#ifdef DEBUG_IFILE
int             debug_ifile;	/* cat the ifile and exit */
#endif
int             cvtlevel;	/* convert to newer file system format */
int             doinglevel1;	/* converting to new cylinder group format */
int             doinglevel2;	/* converting to new inode format */
int             exitonfail;
int             newinofmt;	/* filesystem has new inode format */
int             preen;		/* just fix normal inconsistencies */
char            havesb;		/* superblock has been read */
char            skipclean;	/* skip clean file systems if preening */
int             fsmodified;	/* 1 => write done to file system */
int             fsreadfd;	/* file descriptor for reading file system */
int             fswritefd;	/* file descriptor for writing file system */
int             rerun;		/* rerun fsck.  Only used in non-preen mode */

daddr_t         maxfsblock;	/* number of blocks in the file system */
#ifndef VERBOSE_BLOCKMAP
char           *blockmap;	/* ptr to primary blk allocation map */
#else
ino_t          *blockmap;
#endif
ino_t           maxino;		/* number of inodes in file system */
ino_t           lastino;	/* last inode in use */
char           *statemap;	/* ptr to inode state table */
char           *typemap;	/* ptr to inode type table */
int16_t        *lncntp;		/* ptr to link count table */

ino_t           lfdir;		/* lost & found directory inode number */
char           *lfname;		/* lost & found directory name */
int             lfmode;		/* lost & found directory creation mode */

daddr_t         n_blks;		/* number of blocks in use */
daddr_t         n_files;	/* number of files in use */

struct dinode   zino;
