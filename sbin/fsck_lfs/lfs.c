/* $NetBSD: lfs.c,v 1.35.8.1 2013/02/25 00:28:07 tls Exp $ */
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

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
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
extern u_int32_t lfs_sb_cksum(struct dlfs *);
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
		fs->lfs_avail -= btofsb(fs, bp->b_bcount);
	}
	bp->b_flags |= B_DELWRI | B_LOCKED;
	reassignbuf(bp, bp->b_vp);
	brelse(bp, 0);
	return 0;
}

/*
 * ufs_bmaparray does the bmap conversion, and if requested returns the
 * array of logical blocks which must be traversed to get to a block.
 * Each entry contains the offset into that block that gets you to the
 * next block and the disk address of the block (if it is assigned).
 */
int
ufs_bmaparray(struct lfs * fs, struct uvnode * vp, daddr_t bn, daddr_t * bnp, struct indir * ap, int *nump)
{
	struct inode *ip;
	struct ubuf *bp;
	struct indir a[UFS_NIADDR + 1], *xap;
	daddr_t daddr;
	daddr_t metalbn;
	int error, num;

	ip = VTOI(vp);

	if (bn >= 0 && bn < UFS_NDADDR) {
		if (nump != NULL)
			*nump = 0;
		*bnp = fsbtodb(fs, ip->i_ffs1_db[bn]);
		if (*bnp == 0)
			*bnp = -1;
		return (0);
	}
	xap = ap == NULL ? a : ap;
	if (!nump)
		nump = &num;
	if ((error = ufs_getlbns(fs, vp, bn, xap, nump)) != 0)
		return (error);

	num = *nump;

	/* Get disk address out of indirect block array */
	daddr = ip->i_ffs1_ib[xap->in_off];

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
		bp = getblk(vp, metalbn, fs->lfs_bsize);

		if (!(bp->b_flags & (B_DONE | B_DELWRI))) {
			bp->b_blkno = fsbtodb(fs, daddr);
			bp->b_flags |= B_READ;
			VOP_STRATEGY(bp);
		}
		daddr = ((ufs_daddr_t *) bp->b_data)[xap->in_off];
	}
	if (bp)
		brelse(bp, 0);

	daddr = fsbtodb(fs, (ufs_daddr_t) daddr);
	*bnp = daddr == 0 ? -1 : daddr;
	return (0);
}

/*
 * Create an array of logical block number/offset pairs which represent the
 * path of indirect blocks required to access a data block.  The first "pair"
 * contains the logical block number of the appropriate single, double or
 * triple indirect block and the offset into the inode indirect block array.
 * Note, the logical block number of the inode single/double/triple indirect
 * block appears twice in the array, once with the offset into the i_ffs1_ib and
 * once with the offset into the page itself.
 */
int
ufs_getlbns(struct lfs * fs, struct uvnode * vp, daddr_t bn, struct indir * ap, int *nump)
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
	for (indir = fs->lfs_nindir; indir; indir >>= 1)
		++lognindir;

	/* Determine the number of levels of indirection.  After this loop is
	 * done, blockcnt indicates the number of data blocks possible at the
	 * given level of indirection, and UFS_NIADDR - i is the number of levels
	 * of indirection needed to locate the requested block. */

	bn -= UFS_NDADDR;
	for (lbc = 0, i = UFS_NIADDR;; i--, bn -= blockcnt) {
		if (i == 0)
			return (EFBIG);

		lbc += lognindir;
		blockcnt = (int64_t) 1 << lbc;

		if (bn < blockcnt)
			break;
	}

	/* Calculate the address of the first meta-block. */
	metalbn = -((realbn >= 0 ? realbn : -realbn) - bn + UFS_NIADDR - i);

	/* At each iteration, off is the offset into the bap array which is an
	 * array of disk addresses at the current level of indirection. The
	 * logical block number and the offset in that block are stored into
	 * the argument array. */
	ap->in_lbn = metalbn;
	ap->in_off = off = UFS_NIADDR - i;
	ap->in_exists = 0;
	ap++;
	for (++numlevels; i <= UFS_NIADDR; i++) {
		/* If searching for a meta-data block, quit when found. */
		if (metalbn == realbn)
			break;

		lbc -= lognindir;
		blockcnt = (int64_t) 1 << lbc;
		off = (bn >> lbc) & (fs->lfs_nindir - 1);

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
	return ufs_bmaparray(vp->v_fs, vp, lbn, daddrp, NULL, NULL);
}

/* Search a block for a specific dinode. */
struct ufs1_dinode *
lfs_ifind(struct lfs * fs, ino_t ino, struct ubuf * bp)
{
	struct ufs1_dinode *dip = (struct ufs1_dinode *) bp->b_data;
	struct ufs1_dinode *ldip, *fin;

	fin = dip + INOPB(fs);

	/*
	 * Read the inode block backwards, since later versions of the
	 * inode will supercede earlier ones.  Though it is unlikely, it is
	 * possible that the same inode will appear in the same inode block.
	 */
	for (ldip = fin - 1; ldip >= dip; --ldip)
		if (ldip->di_inumber == ino)
			return (ldip);
	return NULL;
}

/*
 * lfs_raw_vget makes us a new vnode from the inode at the given disk address.
 * XXX it currently loses atime information.
 */
struct uvnode *
lfs_raw_vget(struct lfs * fs, ino_t ino, int fd, ufs_daddr_t daddr)
{
	struct uvnode *vp;
	struct inode *ip;
	struct ufs1_dinode *dip;
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

	ip->i_din.ffs1_din = ecalloc(1, sizeof(*ip->i_din.ffs1_din));

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
		bread(fs->lfs_devvp, fsbtodb(fs, daddr), fs->lfs_ibsize,
		    NULL, 0, &bp);
		bp->b_flags |= B_AGE;
		dip = lfs_ifind(fs, ino, bp);
		if (dip == NULL) {
			brelse(bp, 0);
			free(ip);
			free(vp);
			return NULL;
		}
		memcpy(ip->i_din.ffs1_din, dip, sizeof(*dip));
		brelse(bp, 0);
	}
	ip->i_number = ino;
	/* ip->i_devvp = fs->lfs_devvp; */
	ip->i_lfs = fs;

	ip->i_lfs_effnblks = ip->i_ffs1_blocks;
	ip->i_lfs_osize = ip->i_ffs1_size;
#if 0
	if (fs->lfs_version > 1) {
		ip->i_ffs1_atime = ts.tv_sec;
		ip->i_ffs1_atimensec = ts.tv_nsec;
	}
#endif

	memset(ip->i_lfs_fragsize, 0, UFS_NDADDR * sizeof(*ip->i_lfs_fragsize));
	for (i = 0; i < UFS_NDADDR; i++)
		if (ip->i_ffs1_db[i] != 0)
			ip->i_lfs_fragsize[i] = blksize(fs, ip, i);

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
	ufs_daddr_t daddr;
	struct ubuf *bp;
	IFILE *ifp;

	LFS_IENTRY(ifp, fs, ino, bp);
	daddr = ifp->if_daddr;
	brelse(bp, 0);
	if (daddr <= 0 || dtosn(fs, daddr) >= fs->lfs_nseg)
		return NULL;
	return lfs_raw_vget(fs, ino, fs->lfs_ivnode->v_fd, daddr);
}

/* Check superblock magic number and checksum */
static int
check_sb(struct lfs *fs)
{
	u_int32_t checksum;

	if (fs->lfs_magic != LFS_MAGIC) {
		printf("Superblock magic number (0x%lx) does not match "
		       "expected 0x%lx\n", (unsigned long) fs->lfs_magic,
		       (unsigned long) LFS_MAGIC);
		return 1;
	}
	/* checksum */
	checksum = lfs_sb_cksum(&(fs->lfs_dlfs));
	if (fs->lfs_cksum != checksum) {
		printf("Superblock checksum (%lx) does not match computed checksum (%lx)\n",
		    (unsigned long) fs->lfs_cksum, (unsigned long) checksum);
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
	int error;

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

		error = bread(devvp, sblkno, LFS_SBPAD, NOCRED, 0, &bp);
		fs = ecalloc(1, sizeof(*fs));
		fs->lfs_dlfs = *((struct dlfs *) bp->b_data);
		fs->lfs_devvp = devvp;
		bp->b_flags |= B_INVAL;
		brelse(bp, 0);

		dev_bsize = fs->lfs_fsize >> fs->lfs_fsbtodb;
	
		if (tryalt) {
			error = bread(devvp, fsbtodb(fs, fs->lfs_sboffs[1]),
		    	LFS_SBPAD, NOCRED, 0, &bp);
			altfs = ecalloc(1, sizeof(*altfs));
			altfs->lfs_dlfs = *((struct dlfs *) bp->b_data);
			altfs->lfs_devvp = devvp;
			bp->b_flags |= B_INVAL;
			brelse(bp, 0);
	
			if (check_sb(fs) || fs->lfs_idaddr <= 0) {
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
	if (fs->lfs_version < 2) {
		fs->lfs_sumsize = LFS_V1_SUMMARY_SIZE;
		fs->lfs_ibsize = fs->lfs_bsize;
		fs->lfs_start = fs->lfs_sboffs[0];
		fs->lfs_tstamp = fs->lfs_otstamp;
		fs->lfs_fsbtodb = 0;
	}

	if (!dummy_read) {
		fs->lfs_suflags = emalloc(2 * sizeof(u_int32_t *));
		fs->lfs_suflags[0] = emalloc(fs->lfs_nseg * sizeof(u_int32_t));
		fs->lfs_suflags[1] = emalloc(fs->lfs_nseg * sizeof(u_int32_t));
	}

	if (idaddr == 0)
		idaddr = fs->lfs_idaddr;
	else
		fs->lfs_idaddr = idaddr;
	/* NB: If dummy_read!=0, idaddr==0 here so we get a fake inode. */
	fs->lfs_ivnode = lfs_raw_vget(fs,
		(dummy_read ? LFS_IFILE_INUM : fs->lfs_ifile), devvp->v_fd,
		idaddr);
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
ufs_daddr_t
try_verify(struct lfs *osb, struct uvnode *devvp, ufs_daddr_t goal, int debug)
{
	ufs_daddr_t daddr, odaddr;
	SEGSUM *sp;
	int i, bc, hitclean;
	struct ubuf *bp;
	ufs_daddr_t nodirop_daddr;
	u_int64_t serial;

	bc = 0;
	hitclean = 0;
	odaddr = -1;
	daddr = osb->lfs_offset;
	nodirop_daddr = daddr;
	serial = osb->lfs_serial;
	while (daddr != goal) {
		/*
		 * Don't mistakenly read a superblock, if there is one here.
		 */
		if (sntod(osb, dtosn(osb, daddr)) == daddr) {
			if (daddr == osb->lfs_start)
				daddr += btofsb(osb, LFS_LABELPAD);
			for (i = 0; i < LFS_MAXNUMSB; i++) {
				if (osb->lfs_sboffs[i] < daddr)
					break;
				if (osb->lfs_sboffs[i] == daddr)
					daddr += btofsb(osb, LFS_SBPAD);
			}
		}

		/* Read in summary block */
		bread(devvp, fsbtodb(osb, daddr), osb->lfs_sumsize,
		    NULL, 0, &bp);
		sp = (SEGSUM *)bp->b_data;

		/*
		 * Check for a valid segment summary belonging to our fs.
		 */
		if (sp->ss_magic != SS_MAGIC ||
		    sp->ss_ident != osb->lfs_ident ||
		    sp->ss_serial < serial ||	/* XXX strengthen this */
		    sp->ss_sumsum != cksum(&sp->ss_datasum, osb->lfs_sumsize -
			sizeof(sp->ss_sumsum))) {
			brelse(bp, 0);
			if (debug) {
				if (sp->ss_magic != SS_MAGIC)
					pwarn("pseg at 0x%x: "
					      "wrong magic number\n",
					      (int)daddr);
				else if (sp->ss_ident != osb->lfs_ident)
					pwarn("pseg at 0x%x: "
					      "expected ident %llx, got %llx\n",
					      (int)daddr,
					      (long long)sp->ss_ident,
					      (long long)osb->lfs_ident);
				else if (sp->ss_serial >= serial)
					pwarn("pseg at 0x%x: "
					      "serial %d < %d\n", (int)daddr,
					      (int)sp->ss_serial, (int)serial);
				else
					pwarn("pseg at 0x%x: "
					      "summary checksum wrong\n",
					      (int)daddr);
			}
			break;
		}
		if (debug && sp->ss_serial != serial)
			pwarn("warning, serial=%d ss_serial=%d\n",
				(int)serial, (int)sp->ss_serial);
		++serial;
		bc = check_summary(osb, sp, daddr, debug, devvp, NULL);
		if (bc == 0) {
			brelse(bp, 0);
			break;
		}
		if (debug)
			pwarn("summary good: 0x%x/%d\n", (int)daddr,
			      (int)sp->ss_serial);
		assert (bc > 0);
		odaddr = daddr;
		daddr += btofsb(osb, osb->lfs_sumsize + bc);
		if (dtosn(osb, odaddr) != dtosn(osb, daddr) ||
		    dtosn(osb, daddr) != dtosn(osb, daddr +
			btofsb(osb, osb->lfs_sumsize + osb->lfs_bsize) - 1)) {
			daddr = sp->ss_next;
		}

		/*
		 * Check for the beginning and ending of a sequence of
		 * dirops.  Writes from the cleaner never involve new
		 * information, and are always checkpoints; so don't try
		 * to roll forward through them.  Likewise, psegs written
		 * by a previous roll-forward attempt are not interesting.
		 */
		if (sp->ss_flags & (SS_CLEAN | SS_RFW))
			hitclean = 1;
		if (hitclean == 0 && (sp->ss_flags & SS_CONT) == 0)
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
	ufs_daddr_t daddr;
	struct lfs *osb, *nsb;

	/*
	 * Verify the checkpoint of the newer superblock,
	 * if the timestamp/serial number of the two superblocks is
	 * different.
	 */

	osb = NULL;
	if (debug)
		pwarn("sb0 %lld, sb1 %lld",
		      (long long) sb0->lfs_serial,
		      (long long) sb1->lfs_serial);

	if ((sb0->lfs_version == 1 &&
		sb0->lfs_otstamp != sb1->lfs_otstamp) ||
	    (sb0->lfs_version > 1 &&
		sb0->lfs_serial != sb1->lfs_serial)) {
		if (sb0->lfs_version == 1) {
			if (sb0->lfs_otstamp > sb1->lfs_otstamp) {
				osb = sb1;
				nsb = sb0;
			} else {
				osb = sb0;
				nsb = sb1;
			}
		} else {
			if (sb0->lfs_serial > sb1->lfs_serial) {
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
		daddr = try_verify(osb, devvp, nsb->lfs_offset, debug);

		if (debug)
			printf("done.\n");
		if (daddr == nsb->lfs_offset) {
			pwarn("** Newer checkpoint verified, recovered %lld seconds of data\n",
			    (long long) nsb->lfs_tstamp - (long long) osb->lfs_tstamp);
			sbdirty();
		} else {
			pwarn("** Newer checkpoint invalid, lost %lld seconds of data\n", (long long) nsb->lfs_tstamp - (long long) osb->lfs_tstamp);
		}
		return (daddr == nsb->lfs_offset ? nsb : osb);
	}
	/* Nothing to check */
	return osb;
}

/* Verify a partial-segment summary; return the number of bytes on disk. */
int
check_summary(struct lfs *fs, SEGSUM *sp, ufs_daddr_t pseg_addr, int debug,
	      struct uvnode *devvp, void (func(ufs_daddr_t, FINFO *)))
{
	FINFO *fp;
	int bc;			/* Bytes in partial segment */
	int nblocks;
	ufs_daddr_t seg_addr, daddr;
	ufs_daddr_t *dp, *idp;
	struct ubuf *bp;
	int i, j, k, datac, len;
	long sn;
	u_int32_t *datap;
	u_int32_t ccksum;

	sn = dtosn(fs, pseg_addr);
	seg_addr = sntod(fs, sn);

	/* We've already checked the sumsum, just do the data bounds and sum */

	/* Count the blocks. */
	nblocks = howmany(sp->ss_ninos, INOPB(fs));
	bc = nblocks << (fs->lfs_version > 1 ? fs->lfs_ffshift : fs->lfs_bshift);
	assert(bc >= 0);

	fp = (FINFO *) (sp + 1);
	for (i = 0; i < sp->ss_nfinfo; i++) {
		nblocks += fp->fi_nblocks;
		bc += fp->fi_lastlength + ((fp->fi_nblocks - 1)
					   << fs->lfs_bshift);
		assert(bc >= 0);
		fp = (FINFO *) (fp->fi_blocks + fp->fi_nblocks);
		if (((char *)fp) - (char *)sp > fs->lfs_sumsize)
			return 0;
	}
	datap = emalloc(nblocks * sizeof(*datap));
	datac = 0;

	dp = (ufs_daddr_t *) sp;
	dp += fs->lfs_sumsize / sizeof(ufs_daddr_t);
	dp--;

	idp = dp;
	daddr = pseg_addr + btofsb(fs, fs->lfs_sumsize);
	fp = (FINFO *) (sp + 1);
	for (i = 0, j = 0;
	     i < sp->ss_nfinfo || j < howmany(sp->ss_ninos, INOPB(fs)); i++) {
		if (i >= sp->ss_nfinfo && *idp != daddr) {
			pwarn("Not enough inode blocks in pseg at 0x%" PRIx32
			      ": found %d, wanted %d\n",
			      pseg_addr, j, howmany(sp->ss_ninos, INOPB(fs)));
			if (debug)
				pwarn("*idp=%x, daddr=%" PRIx32 "\n", *idp,
				      daddr);
			break;
		}
		while (j < howmany(sp->ss_ninos, INOPB(fs)) && *idp == daddr) {
			bread(devvp, fsbtodb(fs, daddr), fs->lfs_ibsize,
			    NOCRED, 0, &bp);
			datap[datac++] = ((u_int32_t *) (bp->b_data))[0];
			brelse(bp, 0);

			++j;
			daddr += btofsb(fs, fs->lfs_ibsize);
			--idp;
		}
		if (i < sp->ss_nfinfo) {
			if (func)
				func(daddr, fp);
			for (k = 0; k < fp->fi_nblocks; k++) {
				len = (k == fp->fi_nblocks - 1 ?
				       fp->fi_lastlength
				       : fs->lfs_bsize);
				bread(devvp, fsbtodb(fs, daddr), len,
				    NOCRED, 0, &bp);
				datap[datac++] = ((u_int32_t *) (bp->b_data))[0];
				brelse(bp, 0);
				daddr += btofsb(fs, len);
			}
			fp = (FINFO *) (fp->fi_blocks + fp->fi_nblocks);
		}
	}

	if (datac != nblocks) {
		pwarn("Partial segment at 0x%llx expected %d blocks counted %d\n",
		    (long long) pseg_addr, nblocks, datac);
	}
	ccksum = cksum(datap, nblocks * sizeof(u_int32_t));
	/* Check the data checksum */
	if (ccksum != sp->ss_datasum) {
		pwarn("Partial segment at 0x%" PRIx32 " data checksum"
		      " mismatch: given 0x%x, computed 0x%x\n",
		      pseg_addr, sp->ss_datasum, ccksum);
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
	struct ifile *ifp;
	ino_t new_ino;
	int error;
	int new_gen;
	CLEANERINFO *cip;

	/* Get the head of the freelist. */
	LFS_GET_HEADFREE(fs, cip, cbp, &new_ino);

	/*
	 * Remove the inode from the free list and write the new start
	 * of the free list into the superblock.
	 */
	LFS_IENTRY(ifp, fs, new_ino, bp);
	if (ifp->if_daddr != LFS_UNUSED_DADDR)
		panic("lfs_valloc: inuse inode %d on the free list", new_ino);
	LFS_PUT_HEADFREE(fs, cip, cbp, ifp->if_nextfree);

	new_gen = ifp->if_version; /* version was updated by vfree */
	brelse(bp, 0);

	/* Extend IFILE so that the next lfs_valloc will succeed. */
	if (fs->lfs_freehd == LFS_UNUSED_INUM) {
		if ((error = extend_ifile(fs)) != 0) {
			LFS_PUT_HEADFREE(fs, cip, cbp, new_ino);
			return NULL;
		}
	}

	/* Set superblock modified bit and increment file count. */
        sbdirty();
	++fs->lfs_nfiles;

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
	IFILE *ifp;
	IFILE_V1 *ifp_v1;
	struct ubuf *bp, *cbp;
	daddr_t i, blkno, max;
	ino_t oldlast;
	CLEANERINFO *cip;

	vp = fs->lfs_ivnode;
	ip = VTOI(vp);
	blkno = lblkno(fs, ip->i_ffs1_size);

	lfs_balloc(vp, ip->i_ffs1_size, fs->lfs_bsize, &bp);
	ip->i_ffs1_size += fs->lfs_bsize;
	ip->i_flag |= IN_MODIFIED;
	
	i = (blkno - fs->lfs_segtabsz - fs->lfs_cleansz) *
		fs->lfs_ifpb;
	LFS_GET_HEADFREE(fs, cip, cbp, &oldlast);
	LFS_PUT_HEADFREE(fs, cip, cbp, i);
	max = i + fs->lfs_ifpb;
	fs->lfs_bfree -= btofsb(fs, fs->lfs_bsize);

	if (fs->lfs_version == 1) {
		for (ifp_v1 = (IFILE_V1 *)bp->b_data; i < max; ++ifp_v1) {
			ifp_v1->if_version = 1;
			ifp_v1->if_daddr = LFS_UNUSED_DADDR;
			ifp_v1->if_nextfree = ++i;
		}
		ifp_v1--;
		ifp_v1->if_nextfree = oldlast;
	} else {
		for (ifp = (IFILE *)bp->b_data; i < max; ++ifp) {
			ifp->if_version = 1;
			ifp->if_daddr = LFS_UNUSED_DADDR;
			ifp->if_nextfree = ++i;
		}
		ifp--;
		ifp->if_nextfree = oldlast;
	}
	LFS_PUT_TAILFREE(fs, cip, cbp, max - 1);

	LFS_BWRITE_LOG(bp);

#ifdef IN_FSCK_LFS
	reset_maxino(((ip->i_ffs1_size >> fs->lfs_bshift) - fs->lfs_segtabsz -
		     fs->lfs_cleansz) * fs->lfs_ifpb);
#endif
	return 0;
}

/*
 * Allocate a block, and to inode and filesystem block accounting for it
 * and for any indirect blocks the may need to be created in order for
 * this block to be created.
 *
 * Blocks which have never been accounted for (i.e., which "do not exist")
 * have disk address 0, which is translated by ufs_bmap to the special value
 * UNASSIGNED == -1, as in the historical UFS.
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
	struct indir indirs[UFS_NIADDR+2], *idp;
	daddr_t	lbn, lastblock;
	int bcount;
	int error, frags, i, nsize, osize, num;

	ip = VTOI(vp);
	fs = ip->i_lfs;
	offset = blkoff(fs, startoffset);
	lbn = lblkno(fs, startoffset);

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
	lastblock = lblkno(fs, ip->i_ffs1_size);
	if (lastblock < UFS_NDADDR && lastblock < lbn) {
		osize = blksize(fs, ip, lastblock);
		if (osize < fs->lfs_bsize && osize > 0) {
			if ((error = lfs_fragextend(vp, osize, fs->lfs_bsize,
						    lastblock,
						    (bpp ? &bp : NULL))))
				return (error);
			ip->i_ffs1_size = (lastblock + 1) * fs->lfs_bsize;
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
	if (lbn < UFS_NDADDR && lblkno(fs, ip->i_ffs1_size) <= lbn) {
		osize = blksize(fs, ip, lbn);
		nsize = fragroundup(fs, offset + iosize);
		if (lblktosize(fs, lbn) >= ip->i_ffs1_size) {
			/* Brand new block or fragment */
			frags = numfrags(fs, nsize);
			if (bpp) {
				*bpp = bp = getblk(vp, lbn, nsize);
				bp->b_blkno = UNWRITTEN;
			}
			ip->i_lfs_effnblks += frags;
			fs->lfs_bfree -= frags;
			ip->i_ffs1_db[lbn] = UNWRITTEN;
		} else {
			if (nsize <= osize) {
				/* No need to extend */
				if (bpp && (error = bread(vp, lbn, osize,
				    NOCRED, 0, &bp)))
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

	error = ufs_bmaparray(fs, vp, lbn, &daddr, &indirs[0], &num);
	if (error)
		return (error);

	daddr = (daddr_t)((int32_t)daddr); /* XXX ondisk32 */

	/*
	 * Do byte accounting all at once, so we can gracefully fail *before*
	 * we start assigning blocks.
	 */
        frags = fsbtodb(fs, 1); /* frags = VFSTOUFS(vp->v_mount)->um_seqinc; */
	bcount = 0;
	if (daddr == UNASSIGNED) {
		bcount = frags;
	}
	for (i = 1; i < num; ++i) {
		if (!indirs[i].in_exists) {
			bcount += frags;
		}
	}
	fs->lfs_bfree -= bcount;
	ip->i_lfs_effnblks += bcount;

	if (daddr == UNASSIGNED) {
		if (num > 0 && ip->i_ffs1_ib[indirs[0].in_off] == 0) {
			ip->i_ffs1_ib[indirs[0].in_off] = UNWRITTEN;
		}

		/*
		 * Create new indirect blocks if necessary
		 */
		if (num > 1) {
			idaddr = ip->i_ffs1_ib[indirs[0].in_off];
			for (i = 1; i < num; ++i) {
				ibp = getblk(vp, indirs[i].in_lbn,
				    fs->lfs_bsize);
				if (!indirs[i].in_exists) {
					memset(ibp->b_data, 0, ibp->b_bufsize);
					ibp->b_blkno = UNWRITTEN;
				} else if (!(ibp->b_flags & (B_DELWRI | B_DONE))) {
					ibp->b_blkno = fsbtodb(fs, idaddr);
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
		*bpp = bp = getblk(vp, lbn, blksize(fs, ip, lbn));

	/*
	 * The block we are writing may be a brand new block
	 * in which case we need to do accounting.
	 *
	 * We can tell a truly new block because ufs_bmaparray will say
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
			ip->i_ffs1_db[lbn] = UNWRITTEN;
			break;
		    case 1:
			ip->i_ffs1_ib[indirs[0].in_off] = UNWRITTEN;
			break;
		    default:
			idp = &indirs[num - 1];
			if (bread(vp, idp->in_lbn, fs->lfs_bsize, NOCRED,
				  0, &ibp))
				panic("lfs_balloc: bread bno %lld",
				    (long long)idp->in_lbn);
			/* XXX ondisk32 */
			((int32_t *)ibp->b_data)[idp->in_off] = UNWRITTEN;
			VOP_BWRITE(ibp);
		}
	} else if (bpp && !(bp->b_flags & (B_DONE|B_DELWRI))) {
		/*
		 * Not a brand new block, also not in the cache;
		 * read it in from disk.
		 */
		if (iosize == fs->lfs_bsize)
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
	size_t obufsize;

	ip = VTOI(vp);
	fs = ip->i_lfs;
	frags = (long)numfrags(fs, nsize - osize);
	error = 0;

	/*
	 * If we are not asked to actually return the block, all we need
	 * to do is allocate space for it.  UBC will handle dirtying the
	 * appropriate things and making sure it all goes to disk.
	 * Don't bother to read in that case.
	 */
	if (bpp && (error = bread(vp, lbn, osize, NOCRED, 0, bpp))) {
		brelse(*bpp, 0);
		goto out;
	}

	fs->lfs_bfree -= frags;
	ip->i_lfs_effnblks += frags;
	ip->i_flag |= IN_CHANGE | IN_UPDATE;

	if (bpp) {
		obufsize = (*bpp)->b_bufsize;
		(*bpp)->b_data = erealloc((*bpp)->b_data, nsize);
		(void)memset((*bpp)->b_data + osize, 0, nsize - osize);
	}

    out:
	return (error);
}
