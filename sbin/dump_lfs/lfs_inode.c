/*      $NetBSD: lfs_inode.c,v 1.27 2015/09/21 01:24:58 dholland Exp $ */

/*-
 * Copyright (c) 1980, 1991, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c      8.6 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: lfs_inode.c,v 1.27 2015/09/21 01:24:58 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dump.h"
#undef di_inumber

#define	HASDUMPEDFILE	0x1
#define	HASSUBDIRS	0x2

struct lfs *sblock;

int is_ufs2 = 0;

/*
 * Read the superblock from disk, and check its magic number.
 * Determine whether byte-swapping needs to be done on this filesystem.
 */
int
fs_read_sblock(char *superblock)
{
	union {
		char tbuf[LFS_SBPAD];
		struct lfs lfss;
	} u;

	int ns = 0;
	off_t sboff = LFS_LABELPAD;

	sblock = (struct lfs *)superblock;
	while(1) {
		rawread(sboff, (char *) sblock, LFS_SBPAD);
		if (sblock->lfs_dlfs_u.u_32.dlfs_magic != LFS_MAGIC) {
#ifdef notyet
			if (sblock->lfs_magic == bswap32(LFS_MAGIC)) {
				lfs_sb_swap(sblock, sblock, 0);
				ns = 1;
			} else
#endif
				quit("bad sblock magic number\n");
		}
		if (lfs_fsbtob(sblock, (off_t)lfs_sb_getsboff(sblock, 0)) != sboff) {
			sboff = lfs_fsbtob(sblock, (off_t)lfs_sb_getsboff(sblock, 0));
			continue;
		}
		break;
	}

	/*
	 * Read the secondary and take the older of the two
	 */
	rawread(lfs_fsbtob(sblock, (off_t)lfs_sb_getsboff(sblock, 1)), u.tbuf,
	    sizeof(u.tbuf));
#ifdef notyet
	if (ns)
		lfs_sb_swap(u.tbuf, u.tbuf, 0);
#endif
	if (u.lfss.lfs_dlfs_u.u_32.dlfs_magic != LFS_MAGIC) {
		msg("Warning: secondary superblock at 0x%" PRIx64 " bad magic\n",
			LFS_FSBTODB(sblock, (off_t)lfs_sb_getsboff(sblock, 1)));
	} else {
		if (lfs_sb_getversion(sblock) > 1) {
			if (lfs_sb_getserial(&u.lfss) < lfs_sb_getserial(sblock)) {
				memcpy(sblock, u.tbuf, sizeof(u.tbuf));
				sboff = lfs_fsbtob(sblock, (off_t)lfs_sb_getsboff(sblock, 1));
			}
		} else {
			if (lfs_sb_getotstamp(&u.lfss) < lfs_sb_getotstamp(sblock)) {
				memcpy(sblock, u.tbuf, sizeof(u.tbuf));
				sboff = lfs_fsbtob(sblock, (off_t)lfs_sb_getsboff(sblock, 1));
			}
		}
	}
	if (sboff != LFS_SBPAD) {
		msg("Using superblock at alternate location 0x%lx\n",
		    (unsigned long)(btodb(sboff)));
	}

	return ns;
}

/*
 * Fill in the ufsi struct, as well as the maxino and dev_bsize global
 * variables.
 */
struct ufsi *
fs_parametrize(void)
{
	static struct ufsi ufsi;

	spcl.c_flags = iswap32(iswap32(spcl.c_flags) | DR_NEWINODEFMT);

	ufsi.ufs_dsize = LFS_FSBTODB(sblock, lfs_sb_getsize(sblock));
	if (lfs_sb_getversion(sblock) == 1) 
		ufsi.ufs_dsize = lfs_sb_getsize(sblock) >> lfs_sb_getblktodb(sblock);
	ufsi.ufs_bsize = lfs_sb_getbsize(sblock);
	ufsi.ufs_bshift = lfs_sb_getbshift(sblock);
	ufsi.ufs_fsize = lfs_sb_getfsize(sblock);
	ufsi.ufs_frag = lfs_sb_getfrag(sblock);
	ufsi.ufs_fsatoda = lfs_sb_getfsbtodb(sblock);
	if (lfs_sb_getversion(sblock) == 1)
		ufsi.ufs_fsatoda = 0;
	ufsi.ufs_nindir = lfs_sb_getnindir(sblock);
	ufsi.ufs_inopb = lfs_sb_getinopb(sblock);
	ufsi.ufs_maxsymlinklen = lfs_sb_getmaxsymlinklen(sblock);
	ufsi.ufs_bmask = ~(lfs_sb_getbmask(sblock));
	ufsi.ufs_qbmask = lfs_sb_getbmask(sblock);
	ufsi.ufs_fmask = ~(lfs_sb_getffmask(sblock));
	ufsi.ufs_qfmask = lfs_sb_getffmask(sblock);

	dev_bsize = lfs_sb_getbsize(sblock) >> lfs_sb_getblktodb(sblock);

	return &ufsi;
}

ino_t
fs_maxino(void)
{
	return ((getino(LFS_IFILE_INUM)->dp1.di_size
		   - (lfs_sb_getcleansz(sblock) + lfs_sb_getsegtabsz(sblock))
		   * lfs_sb_getbsize(sblock))
		  / lfs_sb_getbsize(sblock)) * lfs_sb_getifpb(sblock) - 1;
}

void
fs_mapinodes(ino_t maxino, u_int64_t *tapesz, int *anydirskipped)
{
	ino_t ino;

	for (ino = ULFS_ROOTINO; ino < maxino; ino++)
		mapfileino(ino, tapesz, anydirskipped);
}

/*
 * XXX KS - I know there's a better way to do this.
 */
#define BASE_SINDIR (ULFS_NDADDR)
#define BASE_DINDIR (ULFS_NDADDR+LFS_NINDIR(fs))
#define BASE_TINDIR (ULFS_NDADDR+LFS_NINDIR(fs)+LFS_NINDIR(fs)*LFS_NINDIR(fs))

#define D_UNITS (LFS_NINDIR(fs))
#define T_UNITS (LFS_NINDIR(fs)*LFS_NINDIR(fs))

static daddr_t
lfs_bmap(struct lfs *fs, union lfs_dinode *idinode, daddr_t lbn)
{
	daddr_t residue, up;
	int off=0;
	char bp[MAXBSIZE];

	up = UNASSIGNED;	/* XXXGCC -Wunitialized [sh3] */
	
	if(lbn > 0 && lbn > lfs_lblkno(fs, lfs_dino_getsize(fs, idinode))) {
		return UNASSIGNED;
	}
	/*
	 * Indirect blocks: if it is a first-level indirect, pull its
	 * address from the inode; otherwise, call ourselves to find the
	 * address of the parent indirect block, and load that to find
	 * the desired address.
	 */
	if(lbn < 0) {
		lbn *= -1;
		if (lbn == ULFS_NDADDR) {
			/* printf("lbn %d: single indir base\n", -lbn); */
			return lfs_dino_getib(fs, idinode, 0); /* single indirect */
		} else if(lbn == BASE_DINDIR+1) {
			/* printf("lbn %d: double indir base\n", -lbn); */
			return lfs_dino_getib(fs, idinode, 1); /* double indirect */
		} else if(lbn == BASE_TINDIR+2) {
			/* printf("lbn %d: triple indir base\n", -lbn); */
			return lfs_dino_getib(fs, idinode, 2); /* triple indirect */
		}

		/*
		 * Find the immediate parent. This is essentially finding the
		 * residue of modulus, and then rounding accordingly.
		 */
		residue = (lbn-ULFS_NDADDR) % LFS_NINDIR(fs);
		if(residue == 1) {
			/* Double indirect.  Parent is the triple. */
			up = lfs_dino_getib(fs, idinode, 2);
			off = (lbn-2-BASE_TINDIR)/(LFS_NINDIR(fs)*LFS_NINDIR(fs));
			if(up == UNASSIGNED || up == LFS_UNUSED_DADDR)
				return UNASSIGNED;
			/* printf("lbn %d: parent is the triple\n", -lbn); */
			bread(LFS_FSBTODB(sblock, up), bp, lfs_sb_getbsize(sblock));
			return lfs_iblock_get(fs, bp, off);
		} else /* residue == 0 */ {
			/* Single indirect.  Two cases. */
			if(lbn < BASE_TINDIR) {
				/* Parent is the double, simple */
				up = -(BASE_DINDIR) - 1;
				off = (lbn-BASE_DINDIR) / D_UNITS;
				/* printf("lbn %d: parent is %d/%d\n", -lbn, up,off); */
			} else {
				/* Ancestor is the triple, more complex */
				up = ((lbn-BASE_TINDIR) / T_UNITS)
					* T_UNITS + BASE_TINDIR + 1;
				off = (lbn/D_UNITS) - (up/D_UNITS);
				up = -up;
				/* printf("lbn %d: parent is %d/%d\n", -lbn, up,off); */
			}
		}
	} else {
		/* Direct block.  Its parent must be a single indirect. */
		if (lbn < ULFS_NDADDR)
			return lfs_dino_getdb(fs, idinode, lbn);
		else {
			/* Parent is an indirect block. */
			up = -(((lbn-ULFS_NDADDR) / D_UNITS) * D_UNITS + ULFS_NDADDR);
			off = (lbn-ULFS_NDADDR) % D_UNITS;
			/* printf("lbn %d: parent is %d/%d\n", lbn,up,off); */
		}
	}
	up = lfs_bmap(fs,idinode,up);
	if(up == UNASSIGNED || up == LFS_UNUSED_DADDR)
		return UNASSIGNED;
	bread(LFS_FSBTODB(sblock, up), bp, lfs_sb_getbsize(sblock));
	return lfs_iblock_get(fs, bp, off);
}

static IFILE *
lfs_ientry(ino_t ino)
{
	static char ifileblock[MAXBSIZE];
	static daddr_t ifblkno;
	daddr_t lbn;
	daddr_t blkno;
	union dinode *dp;
	union lfs_dinode *ldp;
	unsigned index;
    
	lbn = ino/lfs_sb_getifpb(sblock) + lfs_sb_getcleansz(sblock) + lfs_sb_getsegtabsz(sblock);
	dp = getino(LFS_IFILE_INUM);
	/* XXX this is foolish */
	if (sblock->lfs_is64) {
		ldp = (union lfs_dinode *)&dp->dlp64;
	} else {
		ldp = (union lfs_dinode *)&dp->dlp32;
	}
	blkno = lfs_bmap(sblock, ldp, lbn);
	if (blkno != ifblkno)
		bread(LFS_FSBTODB(sblock, blkno), ifileblock,
		    lfs_sb_getbsize(sblock));
	index = ino % lfs_sb_getifpb(sblock);
	if (sblock->lfs_is64) {
		return (IFILE *) &((IFILE64 *)ifileblock)[index];
	} else if (lfs_sb_getversion(sblock) > 1) {
		return (IFILE *) &((IFILE32 *)ifileblock)[index];
	} else {
		return (IFILE *) &((IFILE_V1 *)ifileblock)[index];
	}
}

/* Search a block for a specific dinode. */
static union lfs_dinode *
lfs_ifind(struct lfs *fs, ino_t ino, void *block)
{
	union lfs_dinode *dip;
	unsigned i, num;

	num = LFS_INOPB(fs);
	for (i = num; i-- > 0; ) {
		dip = DINO_IN_BLOCK(fs, block, i);
		if (lfs_dino_getinumber(fs, dip) == ino)
			return dip;
	}
	return NULL;
}

union dinode *
getino(ino_t inum)
{
	static daddr_t inoblkno;
	daddr_t blkno;
	static union {
		char space[MAXBSIZE];
		struct lfs64_dinode u_64[MAXBSIZE/sizeof(struct lfs64_dinode)];
		struct lfs32_dinode u_32[MAXBSIZE/sizeof(struct lfs32_dinode)];
	} inoblock;
	static union dinode ifile_dinode; /* XXX fill this in */
	static union dinode empty_dinode; /* Always stays zeroed */
	union lfs_dinode *dp;
	ino_t inum2;

	if (inum == LFS_IFILE_INUM) {
		/* Load the ifile inode if not already */
		inum2 = sblock->lfs_is64 ?
			ifile_dinode.dlp64.di_inumber :
			ifile_dinode.dlp32.di_inumber;
		if (inum2 == 0) {
			blkno = lfs_sb_getidaddr(sblock);
			bread(LFS_FSBTODB(sblock, blkno), inoblock.space,
				(int)lfs_sb_getbsize(sblock));
			dp = lfs_ifind(sblock, inum, inoblock.space);
			/* Structure copy */
			if (sblock->lfs_is64) {
				ifile_dinode.dlp64 = dp->u_64;
			} else {
				ifile_dinode.dlp32 = dp->u_32;
			}
		}
		return &ifile_dinode;
	}

	curino = inum;
	blkno = lfs_if_getdaddr(sblock, lfs_ientry(inum));
	if(blkno == LFS_UNUSED_DADDR)
		return &empty_dinode;

	if(blkno != inoblkno) {
		bread(LFS_FSBTODB(sblock, blkno), inoblock.space,
			(int)lfs_sb_getbsize(sblock));
	}
	return (void *)lfs_ifind(sblock, inum, inoblock.space);
}

/*
 * Tell the filesystem not to overwrite any currently dirty segments
 * until we are finished.  (It may still write into clean segments, of course,
 * since we're not using those.)  This is only called when dump_lfs is called
 * with -X, i.e. we are working on a mounted filesystem.
 */
static int root_fd = -1;
char *wrap_mpname;

int
lfs_wrap_stop(char *mpname)
{
	int waitfor = 0;

	root_fd = open(mpname, O_RDONLY, 0);
	if (root_fd < 0)
		return -1;
	wrap_mpname = mpname;
	fcntl(root_fd, LFCNREWIND, -1); /* Ignore return value */
	if (fcntl(root_fd, LFCNWRAPSTOP, &waitfor) < 0) {
		perror("LFCNWRAPSTOP");
		return -1;
	}
	msg("Disabled log wrap on %s\n", mpname);
	return 0;
}

/*
 * Allow the filesystem to continue normal operation.
 * This would happen anyway when we exit; we do it explicitly here
 * to show the message, for the user's benefit.
 */
void
lfs_wrap_go(void)
{
	int waitfor = 0;

	if (root_fd < 0)
		return;

	fcntl(root_fd, LFCNWRAPGO, &waitfor);
	close(root_fd);
	root_fd = -1;
	msg("Re-enabled log wrap on %s\n", wrap_mpname);
}
