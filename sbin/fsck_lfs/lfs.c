/* $NetBSD: lfs.c,v 1.64 2015/10/03 08:28:46 dholland Exp $ */
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ufs_bmap.c	8.8 (Berkeley) 8/11/95
 */


#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#define vnode uvnode
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_inode.h>
#include <ufs/lfs/lfs_accessors.h>
#undef vnode

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"
#include "segwrite.h"
#include "kernelops.h"

#define panic call_panic

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct lfs *);
extern void pwarn(const char *, ...);

extern struct uvnodelst vnodelist;
extern struct uvnodelst getvnodelist[VNODE_HASH_MAX];
extern int nvnodes;

long dev_bsize = DEV_BSIZE;

static int
lfs_fragextend(struct uvnode *, int, int, daddr_t, struct ubuf **);

int fsdirty = 0;
void (*panic_func)(int, const char *, va_list) = my_vpanic;

/*
 * LFS buffer and uvnode operations
 */

int
lfs_vop_strategy(struct ubuf * bp)
{
	int count;

	if (bp->b_flags & B_READ) {
		count = kops.ko_pread(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    bp->b_blkno * dev_bsize);
		if (count == bp->b_bcount)
			bp->b_flags |= B_DONE;
	} else {
		count = kops.ko_pwrite(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    bp->b_blkno * dev_bsize);
		if (count == 0) {
			perror("pwrite");
			return -1;
		}
		bp->b_flags &= ~B_DELWRI;
		reassignbuf(bp, bp->b_vp);
	}
	return 0;
}

int
lfs_vop_bwrite(struct ubuf * bp)
{
	struct lfs *fs;

	fs = bp->b_vp->v_fs;
	if (!(bp->b_flags & B_DELWRI)) {
		lfs_sb_subavail(fs, lfs_btofsb(fs, bp->b_bcount));
	}
	bp->b_flags |= B_DELWRI | B_LOCKED;
	reassignbuf(bp, bp->b_vp);
	brelse(bp, 0);
	return 0;
}

/*
 * ulfs_bmaparray does the bmap conversion, and if requested returns the
 * array of logical blocks which must be traversed to get to a block.
 * Each entry contains the offset into that block that gets you to the
 * next block and the disk address of the block (if it is assigned).
 */
int
ulfs_bmaparray(struct lfs * fs, struct uvnode * vp, daddr_t bn, daddr_t * bnp, struct indir * ap, int *nump)
{
	struct inode *ip;
	struct ubuf *bp;
	struct indir a[ULFS_NIADDR + 1], *xap;
	daddr_t daddr;
	daddr_t metalbn;
	int error, num;

	ip = VTOI(vp);

	if (bn >= 0 && bn < ULFS_NDADDR) {
		if (nump != NULL)
			*nump = 0;
		*bnp = LFS_FSBTODB(fs, lfs_dino_getdb(fs, ip->i_din, bn));
		if (*bnp == 0)
			*bnp = -1;
		return (0);
	}
	xap = ap == NULL ? a : ap;
	if (!nump)
		nump = &num;
	if ((error = ulfs_getlbns(fs, vp, bn, xap, nump)) != 0)
		return (error);

	num = *nump;

	/* Get disk address out of indirect block array */
	daddr = lfs_dino_getib(fs, ip->i_din, xap->in_off);

	for (bp = NULL, ++xap; --num; ++xap) {
		/* Exit the loop if there is no disk address assigned yet and
		 * the indirect block isn't in the cache, or if we were
		 * looking for an indirect block and we've found it. */

		metalbn = xap->in_lbn;
		if ((daddr == 0 && !incore(vp, metalbn)) || metalbn == bn)
			break;
		/*
		 * If we get here, we've either got the block in the cache
		 * or we have a disk address for it, go fetch it.
		 */
		if (bp)
			brelse(bp, 0);

		xap->in_exists = 1;
		bp = getblk(vp, metalbn, lfs_sb_getbsize(fs));

		if (!(bp->b_flags & (B_DONE | B_DELWRI))) {
			bp->b_blkno = LFS_FSBTODB(fs, daddr);
			bp->b_flags |= B_READ;
			VOP_STRATEGY(bp);
		}
		daddr = lfs_iblock_get(fs, bp->b_data, xap->in_off);
	}
	if (bp)
		brelse(bp, 0);

	daddr = LFS_FSBTODB(fs, daddr);
	*bnp = daddr == 0 ? -1 : daddr;
	return (0);
}

/*
 * Create an array of logical block number/offset pairs which represent the
 * path of indirect blocks required to access a data block.  The first "pair"
 * contains the logical block number of the appropriate single, double or
 * triple indirect block and the offset into the inode indirect block array.
 * Note, the logical block number of the inode single/double/triple indirect
 * block appears twice in the array, once with the offset into di_ib and
 * once with the offset into the page itself.
 */
int
ulfs_getlbns(struct lfs * fs, struct uvnode * vp, daddr_t bn, struct indir * ap, int *nump)
{
	daddr_t metalbn, realbn;
	int64_t blockcnt;
	int lbc;
	int i, numlevels, off;
	int lognindir, indir;

	metalbn = 0;    /* XXXGCC -Wuninitialized [sh3] */

	if (nump)
		*nump = 0;
	numlevels = 0;
	realbn = bn;
	if (bn < 0)
		bn = -bn;

	lognindir = -1;
	for (indir = lfs_sb_getnindir(fs); indir; indir >>= 1)
		++lognindir;

	/* Determine the number of levels of indirection.  After this loop is
	 * done, blockcnt indicates the number of data blocks possible at the
	 * given level of indirection, and ULFS_NIADDR - i is the number of levels
	 * of indirection needed to locate the requested block. */

	bn -= ULFS_NDADDR;
	for (lbc = 0, i = ULFS_NIADDR;; i--, bn -= blockcnt) {
		if (i == 0)
			return (EFBIG);

		lbc += lognindir;
		blockcnt = (int64_t) 1 << lbc;

		if (bn < blockcnt)
			break;
	}

	/* Calculate the address of the first meta-block. */
	metalbn = -((realbn >= 0 ? realbn : -realbn) - bn + ULFS_NIADDR - i);

	/* At each iteration, off is the offset into the bap array which is an
	 * array of disk addresses at the current level of indirection. The
	 * logical block number and the offset in that block are stored into
	 * the argument array. */
	ap->in_lbn = metalbn;
	ap->in_off = off = ULFS_NIADDR - i;
	ap->in_exists = 0;
	ap++;
	for (++numlevels; i <= ULFS_NIADDR; i++) {
		/* If searching for a meta-data block, quit when found. */
		if (metalbn == realbn)
			break;

		lbc -= lognindir;
		blockcnt = (int64_t) 1 << lbc;
		off = (bn >> lbc) & (lfs_sb_getnindir(fs) - 1);

		++numlevels;
		ap->in_lbn = metalbn;
		ap->in_off = off;
		ap->in_exists = 0;
		++ap;

		metalbn -= -1 + (off << lbc);
	}
	if (nump)
		*nump = numlevels;
	return (0);
}

int
lfs_vop_bmap(struct uvnode * vp, daddr_t lbn, daddr_t * daddrp)
{
	return ulfs_bmaparray(vp->v_fs, vp, lbn, daddrp, NULL, NULL);
}

/* Search a block for a specific dinode. */
union lfs_dinode *
lfs_ifind(struct lfs *fs, ino_t ino, struct ubuf *bp)
{
	union lfs_dinode *ldip;
	unsigned i, num;

	num = LFS_INOPB(fs);

	/*
	 * Read the inode block backwards, since later versions of the
	 * inode will supercede earlier ones.  Though it is unlikely, it is
	 * possible that the same inode will appear in the same inode block.
	 */
	for (i = num; i-- > 0; ) {
		ldip = DINO_IN_BLOCK(fs, bp->b_data, i);
		if (lfs_dino_getinumber(fs, ldip) == ino)
			return (ldip);
	}
	return NULL;
}

/*
 * lfs_raw_vget makes us a new vnode from the inode at the given disk address.
 * XXX it currently loses atime information.
 */
struct uvnode *
lfs_raw_vget(struct lfs * fs, ino_t ino, int fd, daddr_t daddr)
{
	struct uvnode *vp;
	struct inode *ip;
	union lfs_dinode *dip;
	struct ubuf *bp;
	int i, hash;

	vp = ecalloc(1, sizeof(*vp));
	vp->v_fd = fd;
	vp->v_fs = fs;
	vp->v_usecount = 0;
	vp->v_strategy_op = lfs_vop_strategy;
	vp->v_bwrite_op = lfs_vop_bwrite;
	vp->v_bmap_op = lfs_vop_bmap;
	LIST_INIT(&vp->v_cleanblkhd);
	LIST_INIT(&vp->v_dirtyblkhd);

	ip = ecalloc(1, sizeof(*ip));

	ip->i_din = dip = ecalloc(1, sizeof(*dip));

	/* Initialize the inode -- from lfs_vcreate. */
	ip->inode_ext.lfs = ecalloc(1, sizeof(*ip->inode_ext.lfs));
	vp->v_data = ip;
	/* ip->i_vnode = vp; */
	ip->i_number = ino;
	ip->i_lockf = 0;
	ip->i_lfs_effnblks = 0;
	ip->i_flag = 0;

	/* Load inode block and find inode */
	if (daddr > 0) {
		bread(fs->lfs_devvp, LFS_FSBTODB(fs, daddr), lfs_sb_getibsize(fs),
		    0, &bp);
		bp->b_flags |= B_AGE;
		dip = lfs_ifind(fs, ino, bp);
		if (dip == NULL) {
			brelse(bp, 0);
			free(ip);
			free(vp);
			return NULL;
		}
		lfs_copy_dinode(fs, ip->i_din, dip);
		brelse(bp, 0);
	}
	ip->i_number = ino;
	/* ip->i_devvp = fs->lfs_devvp; */
	ip->i_lfs = fs;

	ip->i_lfs_effnblks = lfs_dino_getblocks(fs, ip->i_din);
	ip->i_lfs_osize = lfs_dino_getsize(fs, ip->i_din);
#if 0
	if (lfs_sb_getversion(fs) > 1) {
		lfs_dino_setatime(fs, ip->i_din, ts.tv_sec);
		lfs_dino_setatimensec(fs, ip->i_din, ts.tv_nsec);
	}
#endif

	memset(ip->i_lfs_fragsize, 0, ULFS_NDADDR * sizeof(*ip->i_lfs_fragsize));
	for (i = 0; i < ULFS_NDADDR; i++)
		if (lfs_dino_getdb(fs, ip->i_din, i) != 0)
			ip->i_lfs_fragsize[i] = lfs_blksize(fs, ip, i);

	++nvnodes;
	hash = ((int)(intptr_t)fs + ino) & (VNODE_HASH_MAX - 1);
	LIST_INSERT_HEAD(&getvnodelist[hash], vp, v_getvnodes);
	LIST_INSERT_HEAD(&vnodelist, vp, v_mntvnodes);

	return vp;
}

static struct uvnode *
lfs_vget(void *vfs, ino_t ino)
{
	struct lfs *fs = (struct lfs *)vfs;
	daddr_t daddr;
	struct ubuf *bp;
	IFILE *ifp;

	LFS_IENTRY(ifp, fs, ino, bp);
	daddr = lfs_if_getdaddr(fs, ifp);
	brelse(bp, 0);
	if (daddr <= 0 || lfs_dtosn(fs, daddr) >= lfs_sb_getnseg(fs))
		return NULL;
	return lfs_raw_vget(fs, ino, fs->lfs_ivnode->v_fd, daddr);
}

/*
 * Check superblock magic number and checksum.
 * Sets lfs_is64 and lfs_dobyteswap.
 */
static int
check_sb(struct lfs *fs)
{
	u_int32_t checksum;
	u_int32_t magic;

	/* we can read the magic out of either the 32-bit or 64-bit dlfs */
	magic = fs->lfs_dlfs_u.u_32.dlfs_magic;

	if (magic != LFS_MAGIC) {
		printf("Superblock magic number (0x%lx) does not match "
		       "expected 0x%lx\n", (unsigned long) magic,
		       (unsigned long) LFS_MAGIC);
		return 1;
	}
	fs->lfs_is64 = 0; /* XXX notyet */
	fs->lfs_dobyteswap = 0; /* XXX notyet */

	/* checksum */
	checksum = lfs_sb_cksum(fs);
	if (lfs_sb_getcksum(fs) != checksum) {
		printf("Superblock checksum (%lx) does not match computed checksum (%lx)\n",
		    (unsigned long) lfs_sb_getcksum(fs), (unsigned long) checksum);
		return 1;
	}
	return 0;
}

/* Initialize LFS library; load superblocks and choose which to use. */
struct lfs *
lfs_init(int devfd, daddr_t sblkno, daddr_t idaddr, int dummy_read, int debug)
{
	struct uvnode *devvp;
	struct ubuf *bp;
	int tryalt;
	struct lfs *fs, *altfs;

	vfs_init();

	devvp = ecalloc(1, sizeof(*devvp));
	devvp->v_fs = NULL;
	devvp->v_fd = devfd;
	devvp->v_strategy_op = raw_vop_strategy;
	devvp->v_bwrite_op = raw_vop_bwrite;
	devvp->v_bmap_op = raw_vop_bmap;
	LIST_INIT(&devvp->v_cleanblkhd);
	LIST_INIT(&devvp->v_dirtyblkhd);

	tryalt = 0;
	if (dummy_read) {
		if (sblkno == 0)
			sblkno = LFS_LABELPAD / dev_bsize;
		fs = ecalloc(1, sizeof(*fs));
		fs->lfs_devvp = devvp;
	} else {
		if (sblkno == 0) {
			sblkno = LFS_LABELPAD / dev_bsize;
			tryalt = 1;
		} else if (debug) {
			printf("No -b flag given, not attempting to verify checkpoint\n");
		}

		dev_bsize = DEV_BSIZE;

		(void)bread(devvp, sblkno, LFS_SBPAD, 0, &bp);
		fs = ecalloc(1, sizeof(*fs));
		__CTASSERT(sizeof(struct dlfs) == sizeof(struct dlfs64));
		memcpy(&fs->lfs_dlfs_u, bp->b_data, sizeof(struct dlfs));
		fs->lfs_devvp = devvp;
		bp->b_flags |= B_INVAL;
		brelse(bp, 0);

		dev_bsize = lfs_sb_getfsize(fs) >> lfs_sb_getfsbtodb(fs);
	
		if (tryalt) {
			(void)bread(devvp, LFS_FSBTODB(fs, lfs_sb_getsboff(fs, 1)),
		    	LFS_SBPAD, 0, &bp);
			altfs = ecalloc(1, sizeof(*altfs));
			memcpy(&altfs->lfs_dlfs_u, bp->b_data,
			       sizeof(struct dlfs));
			altfs->lfs_devvp = devvp;
			bp->b_flags |= B_INVAL;
			brelse(bp, 0);
	
			if (check_sb(fs) || lfs_sb_getidaddr(fs) <= 0) {
				if (debug)
					printf("Primary superblock is no good, using first alternate\n");
				free(fs);
				fs = altfs;
			} else {
				/* If both superblocks check out, try verification */
				if (check_sb(altfs)) {
					if (debug)
						printf("First alternate superblock is no good, using primary\n");
					free(altfs);
				} else {
					if (lfs_verify(fs, altfs, devvp, debug) == fs) {
						free(altfs);
					} else {
						free(fs);
						fs = altfs;
					}
				}
			}
		}
		if (check_sb(fs)) {
			free(fs);
			return NULL;
		}
	}

	/* Compatibility */
	if (lfs_sb_getversion(fs) < 2) {
		lfs_sb_setsumsize(fs, LFS_V1_SUMMARY_SIZE);
		lfs_sb_setibsize(fs, lfs_sb_getbsize(fs));
		lfs_sb_sets0addr(fs, lfs_sb_getsboff(fs, 0));
		lfs_sb_settstamp(fs, lfs_sb_getotstamp(fs));
		lfs_sb_setfsbtodb(fs, 0);
	}

	if (!dummy_read) {
		fs->lfs_suflags = emalloc(2 * sizeof(u_int32_t *));
		fs->lfs_suflags[0] = emalloc(lfs_sb_getnseg(fs) * sizeof(u_int32_t));
		fs->lfs_suflags[1] = emalloc(lfs_sb_getnseg(fs) * sizeof(u_int32_t));
	}

	if (idaddr == 0)
		idaddr = lfs_sb_getidaddr(fs);
	else
		lfs_sb_setidaddr(fs, idaddr);
	/* NB: If dummy_read!=0, idaddr==0 here so we get a fake inode. */
	fs->lfs_ivnode = lfs_raw_vget(fs, LFS_IFILE_INUM,
		devvp->v_fd, idaddr);
	if (fs->lfs_ivnode == NULL)
		return NULL;

	register_vget((void *)fs, lfs_vget);

	return fs;
}

/*
 * Check partial segment validity between fs->lfs_offset and the given goal.
 *
 * If goal == 0, just keep on going until the segments stop making sense,
 * and return the address of the last valid partial segment.
 *
 * If goal != 0, return the address of the first partial segment that failed,
 * or "goal" if we reached it without failure (the partial segment *at* goal
 * need not be valid).
 */
daddr_t
try_verify(struct lfs *osb, struct uvnode *devvp, daddr_t goal, int debug)
{
	daddr_t daddr, odaddr;
	SEGSUM *sp;
	int i, bc, hitclean;
	struct ubuf *bp;
	daddr_t nodirop_daddr;
	u_int64_t serial;

	bc = 0;
	hitclean = 0;
	odaddr = -1;
	daddr = lfs_sb_getoffset(osb);
	nodirop_daddr = daddr;
	serial = lfs_sb_getserial(osb);
	while (daddr != goal) {
		/*
		 * Don't mistakenly read a superblock, if there is one here.
		 */
		if (lfs_sntod(osb, lfs_dtosn(osb, daddr)) == daddr) {
			if (daddr == lfs_sb_gets0addr(osb))
				daddr += lfs_btofsb(osb, LFS_LABELPAD);
			for (i = 0; i < LFS_MAXNUMSB; i++) {
				/* XXX dholland 20150828 I think this is wrong */
				if (lfs_sb_getsboff(osb, i) < daddr)
					break;
				if (lfs_sb_getsboff(osb, i) == daddr)
					daddr += lfs_btofsb(osb, LFS_SBPAD);
			}
		}

		/* Read in summary block */
		bread(devvp, LFS_FSBTODB(osb, daddr), lfs_sb_getsumsize(osb),
		    0, &bp);
		sp = (SEGSUM *)bp->b_data;

		/*
		 * Check for a valid segment summary belonging to our fs.
		 */
		if (lfs_ss_getmagic(osb, sp) != SS_MAGIC ||
		    lfs_ss_getident(osb, sp) != lfs_sb_getident(osb) ||
		    lfs_ss_getserial(osb, sp) < serial ||	/* XXX strengthen this */
		    lfs_ss_getsumsum(osb, sp) !=
		            cksum((char *)sp + lfs_ss_getsumstart(osb),
				  lfs_sb_getsumsize(osb) - lfs_ss_getsumstart(osb))) {
			brelse(bp, 0);
			if (debug) {
				if (lfs_ss_getmagic(osb, sp) != SS_MAGIC)
					pwarn("pseg at 0x%jx: "
					      "wrong magic number\n",
					      (uintmax_t)daddr);
				else if (lfs_ss_getident(osb, sp) != lfs_sb_getident(osb))
					pwarn("pseg at 0x%jx: "
					      "expected ident %jx, got %jx\n",
					      (uintmax_t)daddr,
					      (uintmax_t)lfs_ss_getident(osb, sp),
					      (uintmax_t)lfs_sb_getident(osb));
				else if (lfs_ss_getserial(osb, sp) >= serial)
					pwarn("pseg at 0x%jx: "
					      "serial %d < %d\n",
					      (uintmax_t)daddr,
					      (int)lfs_ss_getserial(osb, sp), (int)serial);
				else
					pwarn("pseg at 0x%jx: "
					      "summary checksum wrong\n",
					      (uintmax_t)daddr);
			}
			break;
		}
		if (debug && lfs_ss_getserial(osb, sp) != serial)
			pwarn("warning, serial=%d ss_serial=%d\n",
				(int)serial, (int)lfs_ss_getserial(osb, sp));
		++serial;
		bc = check_summary(osb, sp, daddr, debug, devvp, NULL);
		if (bc == 0) {
			brelse(bp, 0);
			break;
		}
		if (debug)
			pwarn("summary good: 0x%x/%d\n", (uintmax_t)daddr,
			      (int)lfs_ss_getserial(osb, sp));
		assert (bc > 0);
		odaddr = daddr;
		daddr += lfs_btofsb(osb, lfs_sb_getsumsize(osb) + bc);
		if (lfs_dtosn(osb, odaddr) != lfs_dtosn(osb, daddr) ||
		    lfs_dtosn(osb, daddr) != lfs_dtosn(osb, daddr +
			lfs_btofsb(osb, lfs_sb_getsumsize(osb) + lfs_sb_getbsize(osb)) - 1)) {
			daddr = lfs_ss_getnext(osb, sp);
		}

		/*
		 * Check for the beginning and ending of a sequence of
		 * dirops.  Writes from the cleaner never involve new
		 * information, and are always checkpoints; so don't try
		 * to roll forward through them.  Likewise, psegs written
		 * by a previous roll-forward attempt are not interesting.
		 */
		if (lfs_ss_getflags(osb, sp) & (SS_CLEAN | SS_RFW))
			hitclean = 1;
		if (hitclean == 0 && (lfs_ss_getflags(osb, sp) & SS_CONT) == 0)
			nodirop_daddr = daddr;

		brelse(bp, 0);
	}

	if (goal == 0)
		return nodirop_daddr;
	else
		return daddr;
}

/* Use try_verify to check whether the newer superblock is valid. */
struct lfs *
lfs_verify(struct lfs *sb0, struct lfs *sb1, struct uvnode *devvp, int debug)
{
	daddr_t daddr;
	struct lfs *osb, *nsb;

	/*
	 * Verify the checkpoint of the newer superblock,
	 * if the timestamp/serial number of the two superblocks is
	 * different.
	 */

	osb = NULL;
	if (debug)
		pwarn("sb0 %ju, sb1 %ju",
		      (uintmax_t) lfs_sb_getserial(sb0),
		      (uintmax_t) lfs_sb_getserial(sb1));

	if ((lfs_sb_getversion(sb0) == 1 &&
		lfs_sb_getotstamp(sb0) != lfs_sb_getotstamp(sb1)) ||
	    (lfs_sb_getversion(sb0) > 1 &&
		lfs_sb_getserial(sb0) != lfs_sb_getserial(sb1))) {
		if (lfs_sb_getversion(sb0) == 1) {
			if (lfs_sb_getotstamp(sb0) > lfs_sb_getotstamp(sb1)) {
				osb = sb1;
				nsb = sb0;
			} else {
				osb = sb0;
				nsb = sb1;
			}
		} else {
			if (lfs_sb_getserial(sb0) > lfs_sb_getserial(sb1)) {
				osb = sb1;
				nsb = sb0;
			} else {
				osb = sb0;
				nsb = sb1;
			}
		}
		if (debug) {
			printf("Attempting to verify newer checkpoint...");
			fflush(stdout);
		}
		daddr = try_verify(osb, devvp, lfs_sb_getoffset(nsb), debug);

		if (debug)
			printf("done.\n");
		if (daddr == lfs_sb_getoffset(nsb)) {
			pwarn("** Newer checkpoint verified; recovered %jd seconds of data\n",
			    (intmax_t)(lfs_sb_gettstamp(nsb) - lfs_sb_gettstamp(osb)));
			sbdirty();
		} else {
			pwarn("** Newer checkpoint invalid; lost %jd seconds of data\n", (intmax_t)(lfs_sb_gettstamp(nsb) - lfs_sb_gettstamp(osb)));
		}
		return (daddr == lfs_sb_getoffset(nsb) ? nsb : osb);
	}
	/* Nothing to check */
	return osb;
}

/* Verify a partial-segment summary; return the number of bytes on disk. */
int
check_summary(struct lfs *fs, SEGSUM *sp, daddr_t pseg_addr, int debug,
	      struct uvnode *devvp, void (func(daddr_t, FINFO *)))
{
	FINFO *fp;
	int bc;			/* Bytes in partial segment */
	int nblocks;
	daddr_t daddr;
	IINFO *iibase, *iip;
	struct ubuf *bp;
	int i, j, k, datac, len;
	u_int32_t *datap;
	u_int32_t ccksum;

	/* We've already checked the sumsum, just do the data bounds and sum */

	/* Count the blocks. */
	nblocks = howmany(lfs_ss_getninos(fs, sp), LFS_INOPB(fs));
	bc = nblocks << (lfs_sb_getversion(fs) > 1 ? lfs_sb_getffshift(fs) : lfs_sb_getbshift(fs));
	assert(bc >= 0);

	fp = SEGSUM_FINFOBASE(fs, sp);
	for (i = 0; i < lfs_ss_getnfinfo(fs, sp); i++) {
		nblocks += lfs_fi_getnblocks(fs, fp);
		bc += lfs_fi_getlastlength(fs, fp) + ((lfs_fi_getnblocks(fs, fp) - 1)
					   << lfs_sb_getbshift(fs));
		assert(bc >= 0);
		fp = NEXT_FINFO(fs, fp);
		if (((char *)fp) - (char *)sp > lfs_sb_getsumsize(fs))
			return 0;
	}
	datap = emalloc(nblocks * sizeof(*datap));
	datac = 0;

	iibase = SEGSUM_IINFOSTART(fs, sp);

	iip = iibase;
	daddr = pseg_addr + lfs_btofsb(fs, lfs_sb_getsumsize(fs));
	fp = (FINFO *) (sp + 1);
	for (i = 0, j = 0;
	     i < lfs_ss_getnfinfo(fs, sp) || j < howmany(lfs_ss_getninos(fs, sp), LFS_INOPB(fs)); i++) {
		if (i >= lfs_ss_getnfinfo(fs, sp) && lfs_ii_getblock(fs, iip) != daddr) {
			pwarn("Not enough inode blocks in pseg at 0x%jx: "
			      "found %d, wanted %d\n",
			      pseg_addr, j, howmany(lfs_ss_getninos(fs, sp),
						    LFS_INOPB(fs)));
			if (debug)
				pwarn("iip=0x%jx, daddr=0x%jx\n",
				    (uintmax_t)lfs_ii_getblock(fs, iip),
				    (intmax_t)daddr);
			break;
		}
		while (j < howmany(lfs_ss_getninos(fs, sp), LFS_INOPB(fs)) && lfs_ii_getblock(fs, iip) == daddr) {
			bread(devvp, LFS_FSBTODB(fs, daddr), lfs_sb_getibsize(fs),
			    0, &bp);
			datap[datac++] = ((u_int32_t *) (bp->b_data))[0];
			brelse(bp, 0);

			++j;
			daddr += lfs_btofsb(fs, lfs_sb_getibsize(fs));
			iip = NEXTLOWER_IINFO(fs, iip);
		}
		if (i < lfs_ss_getnfinfo(fs, sp)) {
			if (func)
				func(daddr, fp);
			for (k = 0; k < lfs_fi_getnblocks(fs, fp); k++) {
				len = (k == lfs_fi_getnblocks(fs, fp) - 1 ?
				       lfs_fi_getlastlength(fs, fp)
				       : lfs_sb_getbsize(fs));
				bread(devvp, LFS_FSBTODB(fs, daddr), len,
				    0, &bp);
				datap[datac++] = ((u_int32_t *) (bp->b_data))[0];
				brelse(bp, 0);
				daddr += lfs_btofsb(fs, len);
			}
			fp = NEXT_FINFO(fs, fp);
		}
	}

	if (datac != nblocks) {
		pwarn("Partial segment at 0x%jx expected %d blocks counted %d\n",
		    (intmax_t)pseg_addr, nblocks, datac);
	}
	/* XXX ondisk32 */
	ccksum = cksum(datap, nblocks * sizeof(u_int32_t));
	/* Check the data checksum */
	if (ccksum != lfs_ss_getdatasum(fs, sp)) {
		pwarn("Partial segment at 0x%jx data checksum"
		      " mismatch: given 0x%x, computed 0x%x\n",
		      (uintmax_t)pseg_addr, lfs_ss_getdatasum(fs, sp), ccksum);
		free(datap);
		return 0;
	}
	free(datap);
	assert(bc >= 0);
	return bc;
}

/* print message and exit */
void
my_vpanic(int fatal, const char *fmt, va_list ap)
{
        (void) vprintf(fmt, ap);
	exit(8);
}

void
call_panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
        panic_func(1, fmt, ap);
	va_end(ap);
}

/* Allocate a new inode. */
struct uvnode *
lfs_valloc(struct lfs *fs, ino_t ino)
{
	struct ubuf *bp, *cbp;
	IFILE *ifp;
	ino_t new_ino;
	int error;
	CLEANERINFO *cip;

	/* Get the head of the freelist. */
	LFS_GET_HEADFREE(fs, cip, cbp, &new_ino);

	/*
	 * Remove the inode from the free list and write the new start
	 * of the free list into the superblock.
	 */
	LFS_IENTRY(ifp, fs, new_ino, bp);
	if (lfs_if_getdaddr(fs, ifp) != LFS_UNUSED_DADDR)
		panic("lfs_valloc: inuse inode %d on the free list", new_ino);
	LFS_PUT_HEADFREE(fs, cip, cbp, lfs_if_getnextfree(fs, ifp));

	brelse(bp, 0);

	/* Extend IFILE so that the next lfs_valloc will succeed. */
	if (lfs_sb_getfreehd(fs) == LFS_UNUSED_INUM) {
		if ((error = extend_ifile(fs)) != 0) {
			LFS_PUT_HEADFREE(fs, cip, cbp, new_ino);
			return NULL;
		}
	}

	/* Set superblock modified bit and increment file count. */
        sbdirty();
	lfs_sb_addnfiles(fs, 1);

        return lfs_raw_vget(fs, ino, fs->lfs_devvp->v_fd, 0x0);
}

#ifdef IN_FSCK_LFS
void reset_maxino(ino_t);
#endif

/*
 * Add a new block to the Ifile, to accommodate future file creations.
 */
int
extend_ifile(struct lfs *fs)
{
	struct uvnode *vp;
	struct inode *ip;
	IFILE64 *ifp64;
	IFILE32 *ifp32;
	IFILE_V1 *ifp_v1;
	struct ubuf *bp, *cbp;
	daddr_t i, blkno, max;
	ino_t oldlast;
	CLEANERINFO *cip;

	vp = fs->lfs_ivnode;
	ip = VTOI(vp);
	blkno = lfs_lblkno(fs, lfs_dino_getsize(fs, ip->i_din));

	lfs_balloc(vp, lfs_dino_getsize(fs, ip->i_din), lfs_sb_getbsize(fs), &bp);
	lfs_dino_setsize(fs, ip->i_din,
	    lfs_dino_getsize(fs, ip->i_din) + lfs_sb_getbsize(fs));
	ip->i_flag |= IN_MODIFIED;
	
	i = (blkno - lfs_sb_getsegtabsz(fs) - lfs_sb_getcleansz(fs)) *
		lfs_sb_getifpb(fs);
	LFS_GET_HEADFREE(fs, cip, cbp, &oldlast);
	LFS_PUT_HEADFREE(fs, cip, cbp, i);
	max = i + lfs_sb_getifpb(fs);
	lfs_sb_subbfree(fs, lfs_btofsb(fs, lfs_sb_getbsize(fs)));

	if (fs->lfs_is64) {
		for (ifp64 = (IFILE64 *)bp->b_data; i < max; ++ifp64) {
			ifp64->if_version = 1;
			ifp64->if_daddr = LFS_UNUSED_DADDR;
			ifp64->if_nextfree = ++i;
		}
		ifp64--;
		ifp64->if_nextfree = oldlast;
	} else if (lfs_sb_getversion(fs) > 1) {
		for (ifp32 = (IFILE32 *)bp->b_data; i < max; ++ifp32) {
			ifp32->if_version = 1;
			ifp32->if_daddr = LFS_UNUSED_DADDR;
			ifp32->if_nextfree = ++i;
		}
		ifp32--;
		ifp32->if_nextfree = oldlast;
	} else {
		for (ifp_v1 = (IFILE_V1 *)bp->b_data; i < max; ++ifp_v1) {
			ifp_v1->if_version = 1;
			ifp_v1->if_daddr = LFS_UNUSED_DADDR;
			ifp_v1->if_nextfree = ++i;
		}
		ifp_v1--;
		ifp_v1->if_nextfree = oldlast;
	}
	LFS_PUT_TAILFREE(fs, cip, cbp, max - 1);

	LFS_BWRITE_LOG(bp);

#ifdef IN_FSCK_LFS
	reset_maxino(((lfs_dino_getsize(fs, ip->i_din) >> lfs_sb_getbshift(fs))
		      - lfs_sb_getsegtabsz(fs)
		      - lfs_sb_getcleansz(fs)) * lfs_sb_getifpb(fs));
#endif
	return 0;
}

/*
 * Allocate a block, and to inode and filesystem block accounting for it
 * and for any indirect blocks the may need to be created in order for
 * this block to be created.
 *
 * Blocks which have never been accounted for (i.e., which "do not exist")
 * have disk address 0, which is translated by ulfs_bmap to the special value
 * UNASSIGNED == -1, as in the historical ULFS.
 *
 * Blocks which have been accounted for but which have not yet been written
 * to disk are given the new special disk address UNWRITTEN == -2, so that
 * they can be differentiated from completely new blocks.
 */
int
lfs_balloc(struct uvnode *vp, off_t startoffset, int iosize, struct ubuf **bpp)
{
	int offset;
	daddr_t daddr, idaddr;
	struct ubuf *ibp, *bp;
	struct inode *ip;
	struct lfs *fs;
	struct indir indirs[ULFS_NIADDR+2], *idp;
	daddr_t	lbn, lastblock;
	int bcount;
	int error, frags, i, nsize, osize, num;

	ip = VTOI(vp);
	fs = ip->i_lfs;
	offset = lfs_blkoff(fs, startoffset);
	lbn = lfs_lblkno(fs, startoffset);

	/*
	 * Three cases: it's a block beyond the end of file, it's a block in
	 * the file that may or may not have been assigned a disk address or
	 * we're writing an entire block.
	 *
	 * Note, if the daddr is UNWRITTEN, the block already exists in
	 * the cache (it was read or written earlier).	If so, make sure
	 * we don't count it as a new block or zero out its contents. If
	 * it did not, make sure we allocate any necessary indirect
	 * blocks.
	 *
	 * If we are writing a block beyond the end of the file, we need to
	 * check if the old last block was a fragment.	If it was, we need
	 * to rewrite it.
	 */

	if (bpp)
		*bpp = NULL;

	/* Check for block beyond end of file and fragment extension needed. */
	lastblock = lfs_lblkno(fs, lfs_dino_getsize(fs, ip->i_din));
	if (lastblock < ULFS_NDADDR && lastblock < lbn) {
		osize = lfs_blksize(fs, ip, lastblock);
		if (osize < lfs_sb_getbsize(fs) && osize > 0) {
			if ((error = lfs_fragextend(vp, osize, lfs_sb_getbsize(fs),
						    lastblock,
						    (bpp ? &bp : NULL))))
				return (error);
			lfs_dino_setsize(fs, ip->i_din, (lastblock + 1) * lfs_sb_getbsize(fs));
			ip->i_flag |= IN_CHANGE | IN_UPDATE;
			if (bpp)
				(void) VOP_BWRITE(bp);
		}
	}

	/*
	 * If the block we are writing is a direct block, it's the last
	 * block in the file, and offset + iosize is less than a full
	 * block, we can write one or more fragments.  There are two cases:
	 * the block is brand new and we should allocate it the correct
	 * size or it already exists and contains some fragments and
	 * may need to extend it.
	 */
	if (lbn < ULFS_NDADDR && lfs_lblkno(fs, lfs_dino_getsize(fs, ip->i_din)) <= lbn) {
		osize = lfs_blksize(fs, ip, lbn);
		nsize = lfs_fragroundup(fs, offset + iosize);
		if (lfs_lblktosize(fs, lbn) >= lfs_dino_getsize(fs, ip->i_din)) {
			/* Brand new block or fragment */
			frags = lfs_numfrags(fs, nsize);
			if (bpp) {
				*bpp = bp = getblk(vp, lbn, nsize);
				bp->b_blkno = UNWRITTEN;
			}
			ip->i_lfs_effnblks += frags;
			lfs_sb_subbfree(fs, frags);
			lfs_dino_setdb(fs, ip->i_din, lbn, UNWRITTEN);
		} else {
			if (nsize <= osize) {
				/* No need to extend */
				if (bpp && (error = bread(vp, lbn, osize,
				    0, &bp)))
					return error;
			} else {
				/* Extend existing block */
				if ((error =
				     lfs_fragextend(vp, osize, nsize, lbn,
						    (bpp ? &bp : NULL))))
					return error;
			}
			if (bpp)
				*bpp = bp;
		}
		return 0;
	}

	error = ulfs_bmaparray(fs, vp, lbn, &daddr, &indirs[0], &num);
	if (error)
		return (error);

	daddr = (daddr_t)((int32_t)daddr); /* XXX ondisk32 */

	/*
	 * Do byte accounting all at once, so we can gracefully fail *before*
	 * we start assigning blocks.
	 */
        frags = LFS_FSBTODB(fs, 1); /* frags = VFSTOULFS(vp->v_mount)->um_seqinc; */
	bcount = 0;
	if (daddr == UNASSIGNED) {
		bcount = frags;
	}
	for (i = 1; i < num; ++i) {
		if (!indirs[i].in_exists) {
			bcount += frags;
		}
	}
	lfs_sb_subbfree(fs, bcount);
	ip->i_lfs_effnblks += bcount;

	if (daddr == UNASSIGNED) {
		if (num > 0 && lfs_dino_getib(fs, ip->i_din, indirs[0].in_off) == 0) {
			lfs_dino_setib(fs, ip->i_din, indirs[0].in_off,
				       UNWRITTEN);
		}

		/*
		 * Create new indirect blocks if necessary
		 */
		if (num > 1) {
			idaddr = lfs_dino_getib(fs, ip->i_din, indirs[0].in_off);
			for (i = 1; i < num; ++i) {
				ibp = getblk(vp, indirs[i].in_lbn,
				    lfs_sb_getbsize(fs));
				if (!indirs[i].in_exists) {
					memset(ibp->b_data, 0, ibp->b_bufsize);
					ibp->b_blkno = UNWRITTEN;
				} else if (!(ibp->b_flags & (B_DELWRI | B_DONE))) {
					ibp->b_blkno = LFS_FSBTODB(fs, idaddr);
					ibp->b_flags |= B_READ;
					VOP_STRATEGY(ibp);
				}
				/*
				 * This block exists, but the next one may not.
				 * If that is the case mark it UNWRITTEN to
                                 * keep the accounting straight.
				 */
				/* XXX ondisk32 */
				if (((int32_t *)ibp->b_data)[indirs[i].in_off] == 0)
					((int32_t *)ibp->b_data)[indirs[i].in_off] =
						UNWRITTEN;
				/* XXX ondisk32 */
				idaddr = ((int32_t *)ibp->b_data)[indirs[i].in_off];
				if ((error = VOP_BWRITE(ibp)))
					return error;
			}
		}
	}


	/*
	 * Get the existing block from the cache, if requested.
	 */
	if (bpp)
		*bpp = bp = getblk(vp, lbn, lfs_blksize(fs, ip, lbn));

	/*
	 * The block we are writing may be a brand new block
	 * in which case we need to do accounting.
	 *
	 * We can tell a truly new block because ulfs_bmaparray will say
	 * it is UNASSIGNED.  Once we allocate it we will assign it the
	 * disk address UNWRITTEN.
	 */
	if (daddr == UNASSIGNED) {
		if (bpp) {
			/* Note the new address */
			bp->b_blkno = UNWRITTEN;
		}

		switch (num) {
		    case 0:
			lfs_dino_setdb(fs, ip->i_din, lbn, UNWRITTEN);
			break;
		    case 1:
			lfs_dino_setib(fs, ip->i_din, indirs[0].in_off,
				       UNWRITTEN);
			break;
		    default:
			idp = &indirs[num - 1];
			if (bread(vp, idp->in_lbn, lfs_sb_getbsize(fs), 0, &ibp))
				panic("lfs_balloc: bread bno %lld",
				    (long long)idp->in_lbn);
			/* XXX ondisk32 */
			lfs_iblock_set(fs, ibp->b_data, idp->in_off,
				       UNWRITTEN);
			VOP_BWRITE(ibp);
		}
	} else if (bpp && !(bp->b_flags & (B_DONE|B_DELWRI))) {
		/*
		 * Not a brand new block, also not in the cache;
		 * read it in from disk.
		 */
		if (iosize == lfs_sb_getbsize(fs))
			/* Optimization: I/O is unnecessary. */
			bp->b_blkno = daddr;
		else {
			/*
			 * We need to read the block to preserve the
			 * existing bytes.
			 */
			bp->b_blkno = daddr;
			bp->b_flags |= B_READ;
			VOP_STRATEGY(bp);
			return 0;
		}
	}

	return (0);
}

int
lfs_fragextend(struct uvnode *vp, int osize, int nsize, daddr_t lbn,
               struct ubuf **bpp)
{
	struct inode *ip;
	struct lfs *fs;
	int frags;
	int error;

	ip = VTOI(vp);
	fs = ip->i_lfs;
	frags = (long)lfs_numfrags(fs, nsize - osize);
	error = 0;

	/*
	 * If we are not asked to actually return the block, all we need
	 * to do is allocate space for it.  UBC will handle dirtying the
	 * appropriate things and making sure it all goes to disk.
	 * Don't bother to read in that case.
	 */
	if (bpp && (error = bread(vp, lbn, osize, 0, bpp))) {
		brelse(*bpp, 0);
		goto out;
	}

	lfs_sb_subbfree(fs, frags);
	ip->i_lfs_effnblks += frags;
	ip->i_flag |= IN_CHANGE | IN_UPDATE;

	if (bpp) {
		(*bpp)->b_data = erealloc((*bpp)->b_data, nsize);
		(void)memset((*bpp)->b_data + osize, 0, nsize - osize);
	}

    out:
	return (error);
}
