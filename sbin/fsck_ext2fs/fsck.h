/*	$NetBSD: fsck.h,v 1.16 2016/08/15 18:42:15 jdolecek Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)fsck.h	8.1 (Berkeley) 6/5/93
 */

/*
 * Copyright (c) 1997 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)fsck.h	8.1 (Berkeley) 6/5/93
 */

#define	MAXDUP		10	/* limit on dup blks (per inode) */
#define	MAXBAD		10	/* limit on bad blks (per inode) */
#define	MAXBUFSPACE	80*1024	/* maximum space to allocate to buffers */
#define	INOBUFSIZE	128*1024	/* size of buffer to read inodes in pass1 */

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#define	USTATE	01		/* inode not allocated */
#define	FSTATE	02		/* inode is file */
#define	DSTATE	03		/* inode is directory */
#define	DFOUND	04		/* directory found during descent */
#define	DCLEAR	05		/* directory is to be cleared */
#define	FCLEAR	06		/* file is to be cleared */

/*
 * buffer cache structure.
 */
struct bufarea {
	struct bufarea	*b_next;		/* free list queue */
	struct bufarea	*b_prev;		/* free list queue */
	daddr_t	b_bno;
	int	b_size;
	int	b_errs;
	int	b_flags;
	union {
		char	*b_buf;			/* buffer space */
		/* XXX ondisk32 */
		int32_t	*b_indir;		/* indirect block */
		struct	ext2fs *b_fs;		/* super block */
		struct	ext2_gd *b_cgd;		/* cylinder group descriptor */
		struct	ext2fs_dinode *b_dinode;	/* inode block */
	} b_un;
	char	b_dirty;
};

#define	B_INUSE 1

#define	MINBUFS		5	/* minimum number of buffers required */
struct bufarea bufhead;		/* head of list of other blks in filesys */
struct bufarea sblk;		/* file system superblock */
struct bufarea asblk;		/* first alternate superblock */
struct bufarea *pdirbp;		/* current directory contents */
struct bufarea *pbp;		/* current inode block */
struct bufarea *getdatablk(daddr_t, long);
struct m_ext2fs sblock;

#define	dirty(bp)	(bp)->b_dirty = 1
#define	initbarea(bp) \
	(bp)->b_dirty = 0; \
	(bp)->b_bno = (daddr_t)-1; \
	(bp)->b_flags = 0;

#define	sbdirty()	copyback_sb(&sblk); sblk.b_dirty = 1

enum fixstate {DONTKNOW, NOFIX, FIX, IGNORE};

struct inodesc {
	enum fixstate id_fix;	/* policy on fixing errors */
	int (*id_func)		/* function to be applied to blocks of inode */
	    (struct inodesc *);
	ino_t id_number;	/* inode number described */
	ino_t id_parent;	/* for DATA nodes, their parent */
	daddr_t id_blkno;	/* current block number being examined */
	int id_numfrags;	/* number of frags contained in block */
	quad_t id_filesize;	/* for DATA nodes, the size of the directory */
	int id_loc;		/* for DATA nodes, current location in dir */
	int id_entryno;		/* for DATA nodes, current entry number */
	struct ext2fs_direct *id_dirp;	/* for DATA nodes, ptr to current entry */
	const char *id_name;	/* for DATA nodes, name to find or enter */
	char id_type;		/* type of descriptor, DATA or ADDR */
};
/* file types */
#define	DATA	1
#define	ADDR	2

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
	struct	inoinfo	*i_child, *i_sibling, *i_parentp;
	ino_t	i_number;		/* inode number of this entry */
	ino_t	i_parent;		/* inode number of parent */
	ino_t	i_dotdot;		/* inode number of `..' */
	u_int64_t i_isize;		/* size of inode */
	u_int	i_numblks;		/* size of block array in bytes */
	/* XXX ondisk32 */
	int32_t	i_blks[1];		/* actually longer */
} **inphead, **inpsort;
long numdirs, listmax, inplast;

long	dev_bsize;		/* computed value of DEV_BSIZE */
long	secsize;		/* actual disk sector size */
char	nflag;			/* assume a no response */
char	yflag;			/* assume a yes response */
int	bflag;			/* location of alternate super block */
int	Uflag;			/* resolve user names */
int	debug;			/* output debugging info */
int	preen;			/* just fix normal inconsistencies */
char	havesb;			/* superblock has been read */
char	skipclean;		/* skip clean file systems if preening */
int	fsmodified;		/* 1 => write done to file system */
int	fsreadfd;		/* file descriptor for reading file system */
int	fswritefd;		/* file descriptor for writing file system */
int	rerun;			/* rerun fsck.  Only used in non-preen mode */

daddr_t	maxfsblock;		/* number of blocks in the file system */
char	*blockmap;		/* ptr to primary blk allocation map */
ino_t	maxino;			/* number of inodes in file system */
ino_t	lastino;		/* last inode in use */
char	*statemap;		/* ptr to inode state table */
u_char	*typemap;		/* ptr to inode type table */
int16_t	*lncntp;		/* ptr to link count table */

ino_t	lfdir;			/* lost & found directory inode number */
extern const char *lfname;	/* lost & found directory name */
extern int	lfmode;		/* lost & found directory creation mode */

daddr_t	n_blks;			/* number of blocks in use */
daddr_t	n_files;		/* number of files in use */

#define	clearinode(dp)	(*(dp) = zino)
struct	ext2fs_dinode zino;

#define	setbmap(blkno)	setbit(blockmap, blkno)
#define	testbmap(blkno)	isset(blockmap, blkno)
#define	clrbmap(blkno)	clrbit(blockmap, blkno)

#define	STOP	0x01
#define	SKIP	0x02
#define	KEEPON	0x04
#define	ALTERED	0x08
#define	FOUND	0x10

/*
 * Kernel support for features doesn't imply fsck support
 */
#define EXT2F_COMPAT_SUPP_FSCK		0x00
#define EXT2F_ROCOMPAT_SUPP_FSCK	(EXT2F_ROCOMPAT_SPARSESUPER \
                                         | EXT2F_ROCOMPAT_LARGEFILE \
                                         | EXT2F_ROCOMPAT_HUGE_FILE \
					)
#define EXT2F_INCOMPAT_SUPP_FSCK	(EXT2F_INCOMPAT_FTYPE \
					)

struct ext2fs_dinode *ginode(ino_t);
struct inoinfo *getinoinfo(ino_t);
void getblk(struct bufarea *, daddr_t, long);
ino_t allocino(ino_t, int);
void copyback_sb(struct bufarea*);
daddr_t cgoverhead(int);	/* overhead per cg */
