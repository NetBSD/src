/* $NetBSD: pass0.c,v 1.7 2000/06/14 18:43:59 perseant Exp $	 */

/*
 * Copyright (c) 1998 Konrad E. Schroder.
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
 */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

/* Flags for check_segment */
#define CKSEG_VERBOSE     1
#define CKSEG_IGNORECLEAN 2

extern int      fake_cleanseg;
void
check_segment(int, int, daddr_t, struct lfs *, int,
	      int (*)(struct lfs *, SEGSUM *, daddr_t));

/*
 * Pass 0.  Check the LFS partial segments for valid checksums, correcting
 * if necessary.  Also check for valid offsets for inode and finfo blocks.
 */
/*
 * XXX more could be done here---consistency between inode-held blocks and
 * finfo blocks, for one thing.
 */

#define dbshift (sblock.lfs_bshift - sblock.lfs_fsbtodb)

void
pass0()
{
	daddr_t         daddr;
	IFILE          *ifp;
	struct bufarea *bp;
	ino_t           ino, lastino, nextino, *visited;

	/*
         * Check the inode free list for inuse inodes, and cycles.
	 * Make sure that all free inodes are in fact on the list.
         */
	visited = (ino_t *)malloc(maxino * sizeof(ino_t));
	memset(visited, 0, maxino * sizeof(ino_t));

	lastino = 0;
	ino = sblock.lfs_free;
	while (ino) {
		if (ino >= maxino) {
			printf("! Ino %d out of range (last was %d)\n", ino,
			       lastino);
			break;
		}
		if (visited[ino]) {
			pwarn("! Ino %d already found on the free list!\n",
			       ino);
			if (preen || reply("FIX") == 1) {
				/* lastino can't be zero */
				ifp = lfs_ientry(lastino, &bp);
				ifp->if_nextfree = 0;
				dirty(bp);
				bp->b_flags &= ~B_INUSE;
			}
			break;
		}
		++visited[ino];
		ifp = lfs_ientry(ino, &bp);
		nextino = ifp->if_nextfree;
		daddr = ifp->if_daddr;
		bp->b_flags &= ~B_INUSE;
		if (daddr) {
			pwarn("! Ino %d with daddr 0x%x is on the free list!\n",
			       ino, daddr);
			if (preen || reply("FIX") == 1) {
				if (lastino == 0) {
					sblock.lfs_free = nextino;
					sbdirty();
				} else {
					ifp = lfs_ientry(lastino, &bp);
					ifp->if_nextfree = nextino;
					dirty(bp);
					bp->b_flags &= ~B_INUSE;
				}
				ino = nextino;
				continue;
			}
		}
		lastino = ino;
		ino = nextino;
	}
	/*
	 * Make sure all free inodes were found on the list
	 */
	for (ino = ROOTINO+1; ino < maxino; ++ino) {
		if (visited[ino])
			continue;

		ifp = lfs_ientry(ino, &bp);
		if (ifp->if_daddr) {
			bp->b_flags &= ~B_INUSE;
			continue;
		}

		pwarn("! Ino %d free, but not on the free list\n", ino);
		if (preen || reply("FIX") == 1) {
			ifp->if_nextfree = sblock.lfs_free;
			sblock.lfs_free = ino;
			sbdirty();
			dirty(bp);
		}
		bp->b_flags &= ~B_INUSE;
	}
}

static void
dump_segsum(SEGSUM * sump, daddr_t addr)
{
	printf("Dump partial summary block 0x%x\n", addr);
	printf("\tsumsum:  %x (%d)\n", sump->ss_sumsum, sump->ss_sumsum);
	printf("\tdatasum: %x (%d)\n", sump->ss_datasum, sump->ss_datasum);
	printf("\tnext:    %x (%d)\n", sump->ss_next, sump->ss_next);
	printf("\tcreate:  %x (%d)\n", sump->ss_create, sump->ss_create);
	printf("\tnfinfo:  %x (%d)\n", sump->ss_nfinfo, sump->ss_nfinfo);
	printf("\tninos:   %x (%d)\n", sump->ss_ninos, sump->ss_ninos);
	printf("\tflags:   %c%c\n",
	       sump->ss_flags & SS_DIROP ? 'd' : '-',
	       sump->ss_flags & SS_CONT ? 'c' : '-');
}

void
check_segment(int fd, int segnum, daddr_t addr, struct lfs * fs, int flags, int (*func)(struct lfs *, SEGSUM *, daddr_t))
{
	struct lfs     *sbp;
	SEGSUM         *sump = NULL;
	SEGUSE         *su;
	struct bufarea *bp = NULL;
	int             psegnum = 0, ninos = 0;
	off_t           sum_offset, db_ssize;
	int             bc, su_flags, su_nsums, su_ninos;

	db_ssize = sblock.lfs_ssize << sblock.lfs_fsbtodb;

	su = lfs_gseguse(segnum, &bp);
	su_flags = su->su_flags;
	su_nsums = su->su_nsums;
	su_ninos = su->su_ninos;
	bp->b_flags &= ~B_INUSE;

	/* printf("Seg at 0x%x\n",addr); */
	if ((flags & CKSEG_VERBOSE) && segnum * db_ssize + fs->lfs_sboffs[0] != addr)
		pwarn("WARNING: segment begins at 0x%qx, should be 0x%qx\n",
		      (long long unsigned)addr,
		      (long long unsigned)(segnum * db_ssize + fs->lfs_sboffs[0]));
	sum_offset = ((off_t)addr << dbshift);

	/* If this segment should have a superblock, look for one */
	if (su_flags & SEGUSE_SUPERBLOCK) {
		bp = getddblk(sum_offset >> dbshift, LFS_SBPAD);
		sum_offset += LFS_SBPAD;

		/* check for a superblock -- XXX this is crude */
		sbp = (struct lfs *)(bp->b_un.b_buf);
		if (sbp->lfs_magic == LFS_MAGIC) {
#if 0
			if (sblock.lfs_tstamp == sbp->lfs_tstamp &&
			    memcmp(sbp, &sblock, sizeof(*sbp)) &&
			    (flags & CKSEG_VERBOSE))
				pwarn("SUPERBLOCK MISMATCH SEGMENT %d\n", segnum);
#endif
		} else {
			if (flags & CKSEG_VERBOSE)
				pwarn("SEGMENT %d SUPERBLOCK INVALID\n", segnum);
			/* XXX allow to fix */
		}
		bp->b_flags &= ~B_INUSE;
	}
	/* XXX need to also check whether this one *should* be dirty */
	if ((flags & CKSEG_IGNORECLEAN) && (su_flags & SEGUSE_DIRTY) == 0)
		return;

	while (1) {
		if (su_nsums <= psegnum)
			break;
		bp = getddblk(sum_offset >> dbshift, LFS_SUMMARY_SIZE);
		sump = (SEGSUM *)(bp->b_un.b_buf);
		if (sump->ss_magic != SS_MAGIC) {
			if (flags & CKSEG_VERBOSE)
				printf("PARTIAL SEGMENT %d SEGMENT %d BAD PARTIAL SEGMENT MAGIC (0x%x should be 0x%x)\n",
				 psegnum, segnum, sump->ss_magic, SS_MAGIC);
			bp->b_flags &= ~B_INUSE;
			break;
		}
		if (sump->ss_sumsum != cksum(&sump->ss_datasum, LFS_SUMMARY_SIZE - sizeof(sump->ss_sumsum))) {
			if (flags & CKSEG_VERBOSE) {
				/* Corrupt partial segment */
				pwarn("CORRUPT PARTIAL SEGMENT %d/%d OF SEGMENT %d AT BLK 0x%qx",
				      psegnum, su_nsums, segnum,
				(unsigned long long)sum_offset >> dbshift);
				if (db_ssize < (sum_offset >> dbshift) - addr)
					pwarn(" (+0x%qx/0x%qx)",
					      (unsigned long long)(((sum_offset >> dbshift) - addr) -
								  db_ssize),
					      (unsigned long long)db_ssize);
				else
					pwarn(" (-0x%qx/0x%qx)",
					    (unsigned long long)(db_ssize -
					  ((sum_offset >> dbshift) - addr)),
					      (unsigned long long)db_ssize);
				pwarn("\n");
				dump_segsum(sump, sum_offset >> dbshift);
			}
			/* XXX fix it maybe */
			bp->b_flags &= ~B_INUSE;
			break;	/* XXX could be throwing away data, but if
				 * this segsum is invalid, how to know where
				 * the next summary begins? */
		}
		/*
		 * Good partial segment
		 */
		bc = (*func)(&sblock, sump, (daddr_t)(sum_offset >> dbshift));
		if (bc) {
			sum_offset += LFS_SUMMARY_SIZE + bc;
			ninos += (sump->ss_ninos + INOPB(&sblock) - 1) / INOPB(&sblock);
			psegnum++;
		} else {
			bp->b_flags &= ~B_INUSE;
			break;
		}
		bp->b_flags &= ~B_INUSE;
	}
	if (flags & CKSEG_VERBOSE) {
		if (ninos != su_ninos)
			pwarn("SEGMENT %d has %d ninos, not %d\n",
			      segnum, ninos, su_ninos);
		if (psegnum != su_nsums)
			pwarn("SEGMENT %d has %d summaries, not %d\n",
			      segnum, psegnum, su_nsums);
	}
	return;
}

int
check_summary(struct lfs * fs, SEGSUM * sp, daddr_t pseg_addr)
{
	FINFO          *fp;
	int             bc;	/* Bytes in partial segment */
	int             nblocks;
	daddr_t         seg_addr, *dp, *idp, daddr;
	struct bufarea *bp;
	int             i, j, k, datac, len;
	long            sn;
	u_long         *datap;
	u_int32_t       ccksum;

	sn = datosn(fs, pseg_addr);
	seg_addr = sntoda(fs, sn);

	/*
	 * printf("Pseg at 0x%x, %d inos, %d
	 * finfos\n",addr>>dbshift,sp->ss_ninos,sp->ss_nfinfo);
	 */
	/* We've already checked the sumsum, just do the data bounds and sum */

	/* 1. Count the blocks. */
	nblocks = ((sp->ss_ninos + INOPB(fs) - 1) / INOPB(fs));
	bc = nblocks << fs->lfs_bshift;

	fp = (FINFO *)(sp + 1);
	for (i = 0; i < sp->ss_nfinfo; i++) {
		nblocks += fp->fi_nblocks;
		bc += fp->fi_lastlength + ((fp->fi_nblocks - 1) << fs->lfs_bshift);
		fp = (FINFO *)(fp->fi_blocks + fp->fi_nblocks);
	}
	datap = (u_long *)malloc(nblocks * sizeof(*datap));
	datac = 0;

	dp = (daddr_t *)sp;
	dp += LFS_SUMMARY_SIZE / sizeof(daddr_t);
	dp--;

	idp = dp;
	daddr = pseg_addr + (LFS_SUMMARY_SIZE / dev_bsize);
	fp = (FINFO *)(sp + 1);
	for (i = 0, j = 0; i < sp->ss_nfinfo || j < howmany(sp->ss_ninos, INOPB(fs)); i++) {
		/* printf("*idp=%x, daddr=%x\n", *idp, daddr); */
		if (i >= sp->ss_nfinfo && *idp != daddr) {
			pwarn("Not enough inode blocks in pseg at 0x%x: found %d, wanted %d\n",
			    pseg_addr, j, howmany(sp->ss_ninos, INOPB(fs)));
			pwarn("*idp=%x, daddr=%x\n", *idp, daddr);
			break;
		}
		while (j < howmany(sp->ss_ninos, INOPB(fs)) && *idp == daddr) {
			bp = getddblk(daddr, (1 << fs->lfs_bshift));
			datap[datac++] = ((u_long *)(bp->b_un.b_buf))[0];
			bp->b_flags &= ~B_INUSE;

			++j;
			daddr += (1 << fs->lfs_bshift) / dev_bsize;
			--idp;
		}
		if (i < sp->ss_nfinfo) {
			for (k = 0; k < fp->fi_nblocks; k++) {
				len = (k == fp->fi_nblocks - 1 ? fp->fi_lastlength
				       : (1 << fs->lfs_bshift));
				bp = getddblk(daddr, len);
				datap[datac++] = ((u_long *)(bp->b_un.b_buf))[0];
				bp->b_flags &= ~B_INUSE;
				daddr += len / dev_bsize;
			}
			fp = (FINFO *)(fp->fi_blocks + fp->fi_nblocks);
		}
	}

	if (datac != nblocks) {
		pwarn("Partial segment at 0x%x expected %d blocks counted %d\n",
		      pseg_addr, nblocks, datac);
	}
	ccksum = cksum(datap, nblocks * sizeof(u_long));
	/* Check the data checksum */
	if (ccksum != sp->ss_datasum) {
		pwarn("Partial segment at 0x%x data checksum mismatch: got 0x%x, expected 0x%x\n",
		      pseg_addr, sp->ss_datasum, ccksum);
		/* return 0; */
	}
	return bc;
}
