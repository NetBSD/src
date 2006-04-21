/*	$NetBSD: fsck.h,v 1.45 2006/04/21 15:00:49 skrll Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fsck.h	8.4 (Berkeley) 5/9/95
 */

#include <stdio.h>
#include <machine/bswap.h>

#ifdef PROGRESS
#include "progress.h"
#endif /* PROGRESS */

#define	MAXDUP		10	/* limit on dup blks (per inode) */
#define	MAXBAD		10	/* limit on bad blks (per inode) */
#define	MAXBUFSPACE	40*1024	/* maximum space to allocate to buffers */
#define	INOBUFSIZE	56*1024	/* size of buffer to read inodes in pass1 */

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};
#define       DIP(dp, field) \
	(is_ufs2 ? (dp)->dp2.di_##field : (dp)->dp1.di_##field)

#define       DIP_SET(dp, field, val) do {	\
	if (is_ufs2)				\
		(dp)->dp2.di_##field = (val);	\
	else					\
		(dp)->dp1.di_##field = (val);	\
} while (0)

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

/*
 * Each inode on the filesystem is described by the following structure.
 * The linkcnt is initially set to the value in the inode. Each time it
 * is found during the descent in passes 2, 3, and 4 the count is
 * decremented. Any inodes whose count is non-zero after pass 4 needs to
 * have its link count adjusted by the value remaining in ino_linkcnt.
 */
struct inostat {
	char    ino_state;	/* state of inode, see below */
	char    ino_type;	/* type of inode */
	short   ino_linkcnt;	/* number of links not found */
};
/*
 * Inode states.
 */
#define	USTATE	01		/* inode not allocated */
#define	FSTATE	02		/* inode is file */
#define	DSTATE	03		/* inode is directory */
#define	DFOUND	04		/* directory found during descent */
#define	DCLEAR	05		/* directory is to be cleared */
#define	FCLEAR	06		/* file is to be cleared */
#define	DMARK	07		/* used in propagate()'s traversal algorithm */

/*
 * Inode state information is contained on per cylinder group lists
 * which are described by the following structure.
 */
struct inostatlist {
	long    il_numalloced;  /* number of inodes allocated in this cg */
	struct inostat *il_stat;/* inostat info for this cylinder group */
} *inostathead;


/*
 * buffer cache structure.
 */
struct bufarea {
	struct bufarea *b_next;		/* free list queue */
	struct bufarea *b_prev;		/* free list queue */
	daddr_t b_bno;
	int b_size;
	int b_errs;
	int b_flags;
	union {
		char *b_buf;			/* buffer space */
		int32_t *b_indir1;		/* indirect block */
		int64_t *b_indir2;		/* indirect block */
		struct fs *b_fs;		/* super block */
		struct cg *b_cg;		/* cylinder group */
		struct ufs1_dinode *b_dinode1;	/* UFS1 inode block */
		struct ufs2_dinode *b_dinode2;	/* UFS2 inode block */
		struct appleufslabel *b_appleufs;		/* Apple UFS volume label */
	} b_un;
	char b_dirty;
};

#define       IBLK(bp, i) \
	(is_ufs2 ?  (bp)->b_un.b_indir2[i] : (bp)->b_un.b_indir1[i])

#define       IBLK_SET(bp, i, val) do {		\
	if (is_ufs2)				\
		(bp)->b_un.b_indir2[i] = (val);	\
	else					\
		(bp)->b_un.b_indir1[i] = (val);	\
} while (0)

#define	B_INUSE 1

#define	MINBUFS		5	/* minimum number of buffers required */
struct bufarea bufhead;		/* head of list of other blks in filesys */
struct bufarea sblk;		/* file system superblock */
struct bufarea asblk;		/* file system superblock */
struct bufarea cgblk;		/* cylinder group blocks */
struct bufarea appleufsblk;		/* Apple UFS volume label */
struct bufarea *pdirbp;		/* current directory contents */
struct bufarea *pbp;		/* current inode block */

#define	dirty(bp)	(bp)->b_dirty = 1
#define	initbarea(bp) \
	(bp)->b_dirty = 0; \
	(bp)->b_bno = (daddr_t)-1; \
	(bp)->b_flags = 0;

struct fs *sblock;
struct fs *altsblock;
struct cg *cgrp;
struct fs *sblocksave;
#define	sbdirty() \
	do { \
		memmove(sblk.b_un.b_fs, sblock, SBLOCKSIZE); \
		sb_oldfscompat_write(sblk.b_un.b_fs, sblocksave); \
		if (needswap) \
			ffs_sb_swap(sblk.b_un.b_fs, sblk.b_un.b_fs); \
		sblk.b_dirty = 1; \
	} while (0)
#define	cgdirty()	do {copyback_cg(&cgblk); cgblk.b_dirty = 1;} while (0)

#define appleufsdirty() \
	do { \
		appleufsblk.b_un.b_appleufs->ul_checksum = 0; \
		appleufsblk.b_un.b_appleufs->ul_checksum =  \
			ffs_appleufs_cksum(appleufsblk.b_un.b_appleufs); \
		appleufsblk.b_dirty = 1; \
	} while (0)

enum fixstate {DONTKNOW, NOFIX, FIX, IGNORE};

struct inodesc {
	enum fixstate id_fix;	/* policy on fixing errors */
	int (*id_func)		/* function to be applied to blocks of inode */
	    (struct inodesc *);
	ino_t id_number;	/* inode number described */
	ino_t id_parent;	/* for DATA nodes, their parent */
	daddr_t id_blkno;	/* current block number being examined */
	int id_numfrags;	/* number of frags contained in block */
	int64_t id_filesize;	/* for DATA nodes, the size of the directory */
	int id_loc;		/* for DATA nodes, current location in dir */
	int64_t id_entryno;	/* for DATA nodes, current entry number */
	struct direct *id_dirp;	/* for DATA nodes, ptr to current entry */
	const char *id_name;	/* for DATA nodes, name to find or enter */
	char id_type;		/* type of descriptor, DATA or ADDR */
};
/* file types */
#define	DATA	1
#define	SNAP	2
#define	ADDR	3

/*
 * Linked list of duplicate blocks.
 * 
 * The list is composed of two parts. The first part of the
 * list (from duplist through the node pointed to by muldup)
 * contains a single copy of each duplicate block that has been 
 * found. The second part of the list (from muldup to the end)
 * contains duplicate blocks that have been found more than once.
 * To check if a block has been found as a duplicate it is only
 * necessary to search from duplist through muldup. To find the 
 * total number of times that a block has been found as a duplicate
 * the entire list must be searched for occurrences of the block
 * in question. The following diagram shows a sample list where
 * w (found twice), x (found once), y (found three times), and z
 * (found once) are duplicate block numbers:
 *
 *    w -> y -> x -> z -> y -> w -> y
 *    ^		     ^
 *    |		     |
 * duplist	  muldup
 */
struct dups {
	struct dups *next;
	daddr_t dup;
};
struct dups *duplist;		/* head of dup list */
struct dups *muldup;		/* end of unique duplicate dup block numbers */

/*
 * Linked list of inodes with zero link counts.
 */
struct zlncnt {
	struct zlncnt *next;
	ino_t zlncnt;
};
struct zlncnt *zlnhead;		/* head of zero link count list */

/*
 * Inode cache data structures.
 */
struct inoinfo {
	struct	inoinfo *i_nexthash;	/* next entry in hash chain */
	struct	inoinfo	*i_child, *i_sibling;
	ino_t	i_number;		/* inode number of this entry */
	ino_t	i_parent;		/* inode number of parent */
	ino_t	i_dotdot;		/* inode number of `..' */
	size_t	i_isize;		/* size of inode */
	u_int	i_numblks;		/* size of block array in bytes */
	int64_t i_blks[1];		/* actually longer */
} **inphead, **inpsort;
long numdirs, dirhash, listmax, inplast;

long	dev_bsize;		/* computed value of DEV_BSIZE */
long	secsize;		/* actual disk sector size */
char	nflag;			/* assume a no response */
char	yflag;			/* assume a yes response */
int	bflag;			/* location of alternate super block */
int	debug;			/* output debugging info */
int	cvtlevel;		/* convert to newer file system format */
int	doinglevel1;		/* converting to new cylinder group format */
int	doinglevel2;		/* converting to new inode format */
int	newinofmt;		/* filesystem has new inode format */
char	usedsoftdep;		/* just fix soft dependency inconsistencies */
int	preen;			/* just fix normal inconsistencies */
int	quiet;			/* Don't print anything if clean */
int	forceimage;		/* file system is an image file */
int	doswap;			/* convert byte order */
int	needswap;		/* need to convert byte order in memory */
int	is_ufs2;		/* we're dealing with an UFS2 filesystem */
int	do_blkswap;		/* need to do block addr byteswap */
int	do_dirswap;		/* need to do dir entry byteswap */
int	endian;			/* endian coversion */
int	markclean;		/* mark file system clean when done */
char	havesb;			/* superblock has been read */
char	skipclean;		/* skip clean file systems if preening */
int	fsmodified;		/* 1 => write done to file system */
int	fsreadfd;		/* file descriptor for reading file system */
int	fswritefd;		/* file descriptor for writing file system */
int	rerun;			/* rerun fsck.  Only used in non-preen mode */
char	resolved;		/* cleared if unresolved changes => not clean */
int	isappleufs;		/* filesystem is Apple UFS */

daddr_t maxfsblock;		/* number of blocks in the file system */
char	*blockmap;		/* ptr to primary blk allocation map */
ino_t	maxino;			/* number of inodes in file system */

int	dirblksiz;

extern ino_t	lfdir;		/* lost & found directory inode number */
extern const char *lfname;	/* lost & found directory name */
extern int	lfmode;		/* lost & found directory creation mode */

daddr_t n_blks;		/* number of blocks in use */
ino_t n_files;		/* number of files in use */

long countdirs;

int	got_siginfo;		/* received a SIGINFO */

#define	clearinode(dp) \
do { \
	if (is_ufs2) 			\
		(dp)->dp2 = ufs2_zino;	\
	else				\
		(dp)->dp1 = ufs1_zino;	\
} while (0)

struct	ufs1_dinode ufs1_zino;
struct	ufs2_dinode ufs2_zino;

#define	setbmap(blkno)	setbit(blockmap, blkno)
#define	testbmap(blkno)	isset(blockmap, blkno)
#define	clrbmap(blkno)	clrbit(blockmap, blkno)

#define	STOP	0x01
#define	SKIP	0x02
#define	KEEPON	0x04
#define	ALTERED	0x08
#define	FOUND	0x10

#define	EEXIT	8		/* Standard error exit. */

/* some inline functs to help the byte-swapping mess */
static inline u_int16_t iswap16 (u_int16_t);
static inline u_int32_t iswap32 (u_int32_t);
static inline u_int64_t iswap64 (u_int64_t);

static inline u_int16_t iswap16(x)
	u_int16_t x;
{
	if (needswap)
		return bswap16(x);
	else return x;
}

static inline u_int32_t iswap32(x)
	u_int32_t x;
{
	if (needswap)
		return bswap32(x);
	else return x;
}

static inline u_int64_t iswap64(x)
	u_int64_t x;
{
	if (needswap)
		return bswap64(x);
	else return x;
}
