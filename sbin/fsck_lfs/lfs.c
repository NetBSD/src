/* $NetBSD: lfs.c,v 1.1 2003/03/28 08:09:53 perseant Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "bufcache.h"
#include "vnode.h"
#include "lfs.h"
#include "segwrite.h"

#define panic call_panic

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct dlfs *);

extern struct uvnodelst vnodelist;
extern struct uvnodelst getvnodelist;
extern int nvnodes;

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
		count = pread(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    dbtob(bp->b_blkno));
		if (count == bp->b_bcount)
			bp->b_flags |= B_DONE;
	} else {
		count = pwrite(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    dbtob(bp->b_blkno));
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
	brelse(bp);
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
	struct indir a[NIADDR + 1], *xap;
	daddr_t daddr;
	daddr_t metalbn;
	int error, num;

	ip = VTOI(vp);

	if (bn >= 0 && bn < NDADDR) {
		if (nump != NULL)
			*nump = 0;
		*bnp = fsbtodb(fs, ip->i_ffs_db[bn]);
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
	daddr = ip->i_ffs_ib[xap->in_off];

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
			brelse(bp);

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
		brelse(bp);

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
 * block appears twice in the array, once with the offset into the i_ffs_ib and
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
	 * given level of indirection, and NIADDR - i is the number of levels
	 * of indirection needed to locate the requested block. */

	bn -= NDADDR;
	for (lbc = 0, i = NIADDR;; i--, bn -= blockcnt) {
		if (i == 0)
			return (EFBIG);

		lbc += lognindir;
		blockcnt = (int64_t) 1 << lbc;

		if (bn < blockcnt)
			break;
	}

	/* Calculate the address of the first meta-block. */
	if (realbn >= 0)
		metalbn = -(realbn - bn + NIADDR - i);
	else
		metalbn = -(-realbn - bn + NIADDR - i);

	/* At each iteration, off is the offset into the bap array which is an
	 * array of disk addresses at the current level of indirection. The
	 * logical block number and the offset in that block are stored into
	 * the argument array. */
	ap->in_lbn = metalbn;
	ap->in_off = off = NIADDR - i;
	ap->in_exists = 0;
	ap++;
	for (++numlevels; i <= NIADDR; i++) {
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
struct dinode *
lfs_ifind(struct lfs * fs, ino_t ino, struct ubuf * bp)
{
	struct dinode *dip = (struct dinode *) bp->b_data;
	struct dinode *ldip, *fin;

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
	struct dinode *dip;
	struct ubuf *bp;
	int i;

	vp = (struct uvnode *) malloc(sizeof(*vp));
	memset(vp, 0, sizeof(*vp));
	vp->v_fd = fd;
	vp->v_fs = fs;
	vp->v_usecount = 0;
	vp->v_strategy_op = lfs_vop_strategy;
	vp->v_bwrite_op = lfs_vop_bwrite;
	vp->v_bmap_op = lfs_vop_bmap;

	++nvnodes;
	LIST_INSERT_HEAD(&getvnodelist, vp, v_getvnodes);
	LIST_INSERT_HEAD(&vnodelist, vp, v_mntvnodes);

	vp->v_data = ip = (struct inode *) malloc(sizeof(*ip));
	memset(ip, 0, sizeof(*ip));

	/* Initialize the inode -- from lfs_vcreate. */
	ip->inode_ext.lfs = malloc(sizeof(struct lfs_inode_ext));
	memset(ip->inode_ext.lfs, 0, sizeof(struct lfs_inode_ext));
	vp->v_data = ip;
	/* ip->i_vnode = vp; */
	ip->i_number = ino;
	ip->i_lockf = 0;
	ip->i_diroff = 0;
	ip->i_ffs_mode = 0;
	ip->i_ffs_size = 0;
	ip->i_ffs_blocks = 0;
	ip->i_lfs_effnblks = 0;
	ip->i_flag = 0;

	/* Load inode block and find inode */
	bread(fs->lfs_unlockvp, fsbtodb(fs, daddr), fs->lfs_ibsize, NULL, &bp);
	bp->b_flags |= B_AGE;
	dip = lfs_ifind(fs, ino, bp);
	if (dip == NULL) {
		brelse(bp);
		free(vp);
		return NULL;
	}
	memcpy(&ip->i_din.ffs_din, dip, sizeof(*dip));
	brelse(bp);
	ip->i_number = ino;
	/* ip->i_devvp = fs->lfs_unlockvp; */
	ip->i_lfs = fs;

	ip->i_ffs_effnlink = ip->i_ffs_nlink;
	ip->i_lfs_effnblks = ip->i_ffs_blocks;
	ip->i_lfs_osize = ip->i_ffs_size;
#if 0
	if (fs->lfs_version > 1) {
		ip->i_ffs_atime = ts.tv_sec;
		ip->i_ffs_atimensec = ts.tv_nsec;
	}
#endif

	memset(ip->i_lfs_fragsize, 0, NDADDR * sizeof(*ip->i_lfs_fragsize));
	for (i = 0; i < NDADDR; i++)
		if (ip->i_ffs_db[i] != 0)
			ip->i_lfs_fragsize[i] = blksize(fs, ip, i);

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
	brelse(bp);
	if (daddr == 0)
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
lfs_init(int devfd, daddr_t sblkno, daddr_t idaddr, int debug)
{
	struct uvnode *devvp;
	struct ubuf *bp;
	int tryalt;
	struct lfs *fs, *altfs;
	int error;

	vfs_init();

	devvp = (struct uvnode *) malloc(sizeof(*devvp));
	devvp->v_fs = NULL;
	devvp->v_fd = devfd;
	devvp->v_strategy_op = raw_vop_strategy;
	devvp->v_bwrite_op = raw_vop_bwrite;
	devvp->v_bmap_op = raw_vop_bmap;

	tryalt = 0;
	if (sblkno == 0) {
		sblkno = btodb(LFS_LABELPAD);
		tryalt = 1;
	} else if (debug) {
		printf("No -b flag given, not attempting to verify checkpoint\n");
	}
	error = bread(devvp, sblkno, LFS_SBPAD, NOCRED, &bp);
	fs = (struct lfs *) malloc(sizeof(*fs));
	*fs = *((struct lfs *) bp->b_data);
	fs->lfs_unlockvp = devvp;
	bp->b_flags |= B_INVAL;
	brelse(bp);

	if (tryalt) {
		error = bread(devvp, fsbtodb(fs, fs->lfs_sboffs[1]),
		    LFS_SBPAD, NOCRED, &bp);
		altfs = (struct lfs *) malloc(sizeof(*fs));
		*altfs = *((struct lfs *) bp->b_data);
		altfs->lfs_unlockvp = devvp;
		bp->b_flags |= B_INVAL;
		brelse(bp);

		if (check_sb(fs)) {
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
	/* Compatibility */
	if (fs->lfs_version < 2) {
		fs->lfs_sumsize = LFS_V1_SUMMARY_SIZE;
		fs->lfs_ibsize = fs->lfs_bsize;
		fs->lfs_start = fs->lfs_sboffs[0];
		fs->lfs_tstamp = fs->lfs_otstamp;
		fs->lfs_fsbtodb = 0;
	}
	fs->lfs_suflags = (u_int32_t **) malloc(2 * sizeof(u_int32_t *));
	fs->lfs_suflags[0] = (u_int32_t *) malloc(fs->lfs_nseg * sizeof(u_int32_t));
	fs->lfs_suflags[1] = (u_int32_t *) malloc(fs->lfs_nseg * sizeof(u_int32_t));

	if (idaddr == 0)
		idaddr = fs->lfs_idaddr;
	fs->lfs_ivnode = lfs_raw_vget(fs, fs->lfs_ifile, devvp->v_fd, idaddr);

	register_vget((void *)fs, lfs_vget);

	return fs;
}

/*
 * Check partial segment validity between fs->lfs_offset and the given goal.
 * If goal == 0, just keep on going until the segments stop making sense.
 * Return the address of the first partial segment that failed.
 */
ufs_daddr_t
try_verify(struct lfs *osb, struct uvnode *devvp, ufs_daddr_t goal, int debug)
{
	ufs_daddr_t daddr, odaddr;
	SEGSUM *sp;
	int bc, flag;
	struct ubuf *bp;
	ufs_daddr_t nodirop_daddr;
	u_int64_t serial;

	daddr = osb->lfs_offset;
	nodirop_daddr = daddr;
	serial = osb->lfs_serial;
	while (daddr != goal) {
		flag = 0;
oncemore:
		/* Read in summary block */
		bread(devvp, fsbtodb(osb, daddr), osb->lfs_sumsize, NULL, &bp);
		sp = (SEGSUM *)bp->b_data;

		/*
		 * Could be a superblock instead of a segment summary.
		 * XXX should use gseguse, but right now we need to do more
		 * setup before we can...fix this
		 */
		if (sp->ss_magic != SS_MAGIC ||
		    sp->ss_ident != osb->lfs_ident ||
		    sp->ss_serial < serial ||
		    sp->ss_sumsum != cksum(&sp->ss_datasum, osb->lfs_sumsize -
			sizeof(sp->ss_sumsum))) {
			brelse(bp);
			if (flag == 0) {
				flag = 1;
				daddr += btofsb(osb, LFS_SBPAD);
				goto oncemore;
			}
			break;
		}
		++serial;
		bc = check_summary(osb, sp, daddr, debug, devvp, NULL);
		if (bc == 0) {
			brelse(bp);
			break;
		}
		assert (bc > 0);
		odaddr = daddr;
		daddr += btofsb(osb, osb->lfs_sumsize + bc);
		if (dtosn(osb, odaddr) != dtosn(osb, daddr) ||
		    dtosn(osb, daddr) != dtosn(osb, daddr +
			btofsb(osb, osb->lfs_sumsize + osb->lfs_bsize))) {
			daddr = sp->ss_next;
		}
		if (!(sp->ss_flags & SS_CONT))
			nodirop_daddr = daddr;
		brelse(bp);
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

	if (debug)
		printf("sb0 %lld, sb1 %lld\n", (long long) sb0->lfs_serial,
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
			warnx("** Newer checkpoint verified, recovered %lld seconds of data\n",
			    (long long) nsb->lfs_tstamp - (long long) osb->lfs_tstamp);
			sbdirty();
		} else {
			warnx("** Newer checkpoint invalid, lost %lld seconds of data\n", (long long) nsb->lfs_tstamp - (long long) osb->lfs_tstamp);
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
	}
	datap = (u_int32_t *) malloc(nblocks * sizeof(*datap));
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
			warnx("Not enough inode blocks in pseg at 0x" PRIx32
			      ": found %d, wanted %d\n",
			      pseg_addr, j, howmany(sp->ss_ninos, INOPB(fs)));
			if (debug)
				warnx("*idp=%x, daddr=%" PRIx32 "\n", *idp,
				      daddr);
			break;
		}
		while (j < howmany(sp->ss_ninos, INOPB(fs)) && *idp == daddr) {
			bread(devvp, fsbtodb(fs, daddr), fs->lfs_ibsize, NOCRED, &bp);
			datap[datac++] = ((u_int32_t *) (bp->b_data))[0];
			brelse(bp);

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
				bread(devvp, fsbtodb(fs, daddr), len, NOCRED, &bp);
				datap[datac++] = ((u_int32_t *) (bp->b_data))[0];
				brelse(bp);
				daddr += btofsb(fs, len);
			}
			fp = (FINFO *) (fp->fi_blocks + fp->fi_nblocks);
		}
	}

	if (datac != nblocks) {
		warnx("Partial segment at 0x%llx expected %d blocks counted %d\n",
		    (long long) pseg_addr, nblocks, datac);
	}
	ccksum = cksum(datap, nblocks * sizeof(u_int32_t));
	/* Check the data checksum */
	if (ccksum != sp->ss_datasum) {
		warnx("Partial segment at 0x%" PRIx32 " data checksum"
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
