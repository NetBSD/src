/* $NetBSD: fsck.h,v 1.5.8.1 2002/06/02 15:27:57 tv Exp $	 */

/*
 * Copyright (c) 1997
 *      Konrad Schroder.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifdef KS_DEBUG
#include <err.h>
#define debug_printf warn
#else
#define debug_printf
#endif

#define	MAXDUP		10	/* limit on dup blks (per inode) */
#define	MAXBAD		10	/* limit on bad blks (per inode) */
#define	MAXBUFSPACE	40*1024	/* maximum space to allocate to buffers */
#define	INOBUFSIZE	56*1024	/* size of buffer to read inodes in pass1 */

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
	struct bufarea *b_next;	/* free list queue */
	struct bufarea *b_prev;	/* free list queue */
	daddr_t         b_bno;
	int             b_size;
	int             b_errs;
	int             b_flags;
	union {
		char           *b_buf;	/* buffer space */
		daddr_t        *b_indir;	/* indirect block */
		struct lfs     *b_fs;	/* super block */
		struct cg      *b_cg;	/* cylinder group */
		struct dinode  *b_dinode;	/* inode block */
	}               b_un;
	char            b_dirty;
};

#define	B_INUSE 1

#define	MINBUFS		5	/* minimum number of buffers required */

#define	dirty(bp)	(bp)->b_dirty = 1
#define	initbarea(bp) \
	(bp)->b_dirty = 0; \
	(bp)->b_bno = (daddr_t)-1; \
	(bp)->b_flags = 0;

#define	sblock		(*sblk.b_un.b_fs)
#define ifblock         (*iblk.b_un.b_dinode)
#define	sbdirty() do {							\
	sblk.b_dirty = 1;						\
	sblock.lfs_dlfs.dlfs_cksum = lfs_sb_cksum(&(sblock.lfs_dlfs));	\
} while(0)

enum fixstate {
	DONTKNOW, NOFIX, FIX, IGNORE
};

struct inodesc {
	enum fixstate   id_fix;		/* policy on fixing errors */
	int             (*id_func)(struct inodesc *);  /* function to be
					 * applied to blocks of inode */
	ino_t           id_number;	/* inode number described */
	ino_t           id_parent;	/* for DATA nodes, their parent */
	daddr_t         id_blkno;	/* current block number being
					 * examined */
	daddr_t         id_lblkno;	/* current logical block number */
	int             id_numfrags;	/* number of frags contained in block */
	quad_t          id_filesize;	/* for DATA nodes, the size of the
					 * directory */
	int             id_loc;		/* for DATA nodes, current location
					 * in dir */
	int             id_entryno;	/* for DATA nodes, current entry
					 * number */
	struct direct  *id_dirp;	/* for DATA nodes, ptr to current
					 * entry */
	char           *id_name;	/* for DATA nodes, name to find or
					 * enter */
	char            id_type;	/* type of descriptor, DATA or ADDR */
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
 * the entire list must be searched for occurences of the block
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
	struct dups    *next;
	daddr_t         dup;
};

/*
 * Linked list of inodes with zero link counts.
 */
struct zlncnt {
	struct zlncnt  *next;
	ino_t           zlncnt;
};

/*
 * Inode cache data structures.
 */
struct inoinfo {
	struct inoinfo *i_nexthash;	/* next entry in hash chain */
	struct inoinfo *i_child, *i_sibling, *i_parentp;
	ino_t           i_number;	/* inode number of this entry */
	ino_t           i_parent;	/* inode number of parent */
	ino_t           i_dotdot;	/* inode number of `..' */
	size_t          i_isize;	/* size of inode */
	u_int           i_numblks;	/* size of block array in bytes */
	daddr_t         i_blks[1];	/* actually longer */
} **inphead, **inpsort;

#define	clearinode(dp)	(*(dp) = zino)

#ifndef VERBOSE_BLOCKMAP
#define	setbmap(blkno)	setbit(blockmap, blkno)
#define	testbmap(blkno)	isset(blockmap, blkno)
#define	clrbmap(blkno)	clrbit(blockmap, blkno)
#else
#define	setbmap(blkno,ino)	if(blkno > maxfsblock)raise(1); else blockmap[blkno] = ino
#define	testbmap(blkno)		blockmap[blkno]
#define	clrbmap(blkno)		blockmap[blkno] = 0
#endif

#define	STOP	0x01
#define	SKIP	0x02
#define	KEEPON	0x04
#define	ALTERED	0x08
#define	FOUND	0x10

ino_t           allocino(ino_t, int);
int             ino_to_fsba(struct lfs *, ino_t);
struct bufarea *getfileblk(struct lfs *, struct dinode *, ino_t);
struct dinode  *ginode(ino_t);
struct dinode  *lfs_ginode(ino_t);
struct dinode  *lfs_difind(struct lfs *, ino_t, struct dinode *);
struct ifile   *lfs_ientry(ino_t, struct bufarea **);
struct inoinfo *getinoinfo(ino_t);
void            getblk(struct bufarea *, daddr_t, long);
void            getdblk(struct bufarea *, daddr_t, long);
int             check_summary(struct lfs *, SEGSUM *, daddr_t);
SEGUSE         *lfs_gseguse(int, struct bufarea **);
daddr_t         lfs_ino_daddr(ino_t);

#include "fsck_vars.h"
