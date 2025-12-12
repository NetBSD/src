/* $NetBSD: pass5.c,v 1.39 2025/12/12 15:53:57 perseant Exp $	 */

/*-
 * Copyright (c) 2000, 2003 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#define vnode uvnode
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>
#undef vnode

#include <string.h>

#include "bufcache.h"
#include "lfs_user.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern int Sflag;

void
pass5(void)
{
	SEGUSE *su;
	struct ubuf *bp;
	int i, nsb, curr, labelcorrect, mfs;
	daddr_t bb;		/* total number of used blocks (lower bound) */
	daddr_t ubb;		/* upper bound number of used blocks */
	daddr_t avail;		/* blocks available for writing */
	daddr_t bfree_observed, bfree_lb, bfree_ub; /* blocks nominally free */
	unsigned long dmeta;	/* blocks in segsums and inodes */
	int nclean;		/* clean segments */
	size_t labelskew;
	int diddirty;

	/*
	 * Check segment holdings against actual holdings.  Check for
	 * "clean" segments that contain live data.  If we are only
	 * rolling forward, we can't check the segment holdings, but
	 * we can still check the cleanerinfo data.
	 */
	nclean = 0;
	avail = 0;
	bb = ubb = 0;
	dmeta = 0;
	nsb = curr = labelcorrect = mfs = 0;
	for (i = 0; i < lfs_sb_getnseg(fs); i++) {
		diddirty = 0;
		LFS_SEGENTRY(su, fs, i, bp);
		if (!preen && !(su->su_flags & SEGUSE_DIRTY) &&
		    seg_table[i].su_nbytes > 0) {
			pwarn("CLEAN SEGMENT %d CONTAINS %d BYTES\n",
			    i, seg_table[i].su_nbytes);
			if (reply("MARK SEGMENT DIRTY")) {
				su->su_flags |= SEGUSE_DIRTY;
				++diddirty;
			}
		}
		if (!preen && su->su_nbytes != seg_table[i].su_nbytes) {
			pwarn("SEGMENT %d CLAIMS %d BYTES BUT HAS %d",
			    i, su->su_nbytes, seg_table[i].su_nbytes);
			if ((int32_t)su->su_nbytes >
			    (int32_t)seg_table[i].su_nbytes)
				pwarn(" (HIGH BY %d)\n", su->su_nbytes -
				    seg_table[i].su_nbytes);
			else
				pwarn(" (LOW BY %d)\n", -su->su_nbytes +
				    seg_table[i].su_nbytes);
			if (reply("FIX")) {
				su->su_nbytes = seg_table[i].su_nbytes;
				++diddirty;
			}
		}
		if (su->su_flags & SEGUSE_DIRTY) {
			bb += lfs_btofsb(fs, su->su_nbytes +
			    su->su_nsums * lfs_sb_getsumsize(fs));
			ubb += lfs_btofsb(fs, su->su_nbytes +
			    su->su_nsums * lfs_sb_getsumsize(fs) +
			    su->su_ninos * lfs_sb_getibsize(fs));
			dmeta += lfs_btofsb(fs,
			    lfs_sb_getsumsize(fs) * su->su_nsums);
			dmeta += lfs_btofsb(fs,
			    lfs_sb_getibsize(fs) * su->su_ninos);
		} else {
			nclean++;
			avail += lfs_segtod(fs, 1);
			if (su->su_flags & SEGUSE_SUPERBLOCK)
				++nsb;
			if (i == 0 && lfs_sb_getversion(fs) > 1 &&
			    lfs_sb_gets0addr(fs) < lfs_btofsb(fs, LFS_LABELPAD))
				labelcorrect = lfs_btofsb(fs, LFS_LABELPAD) -
				    lfs_sb_gets0addr(fs);
		}
		if (diddirty)
			VOP_BWRITE(bp);
		else
			brelse(bp, 0);
	}

	/* Also may be available bytes in current seg */
	i = lfs_dtosn(fs, lfs_sb_getoffset(fs));
	curr = lfs_sntod(fs, i + 1) - lfs_sb_getoffset(fs);
	/* But do not count minfreesegs */
	mfs = lfs_segtod(fs, (lfs_sb_getminfreeseg(fs) -
			      (lfs_sb_getminfreeseg(fs) / 2)));

	avail = nclean * lfs_segtod(fs, 1);
	avail -= nsb * lfs_btofsb(fs, LFS_SBPAD);
	avail -= labelcorrect;
	avail += curr;
	avail -= mfs;

	/* Note we may have bytes to write yet */
	avail -= lfs_btofsb(fs, locked_queue_bytes);

	if (debug)
		pwarn("avail := clean %jd*%jd - sb %jd*%jd - lbl %jd"
		      "+ curr %jd - mfs %jd - locked %jd = %jd\n",
		      (intmax_t)nclean,
		      (intmax_t)lfs_segtod(fs, 1),
		      (intmax_t)nsb,
		      (intmax_t)lfs_btofsb(fs, LFS_SBPAD),
		      (intmax_t)labelcorrect,
		      (intmax_t)curr,
		      (intmax_t)mfs,
		      (intmax_t)lfs_btofsb(fs, locked_queue_bytes),
		      (intmax_t)avail);

	if (idaddr)
		pwarn("NOTE: when using -i, expect discrepancies in dmeta,"
		      " avail, nclean, bfree\n");
	if (dmeta != lfs_sb_getdmeta(fs)) {
		pwarn("DMETA GIVEN AS %d, SHOULD BE %ld\n",
		    lfs_sb_getdmeta(fs), dmeta);
		if (preen || reply("FIX")) {
			lfs_sb_setdmeta(fs, dmeta);
			sbdirty();
		}
	}
	if (avail != lfs_sb_getavail(fs) && !aflag) {
		pwarn("AVAIL GIVEN AS %jd, SHOULD BE %jd\n",
		      (intmax_t)lfs_sb_getavail(fs), (intmax_t)avail);
		if (preen || reply("FIX")) {
			lfs_sb_setavail(fs, avail);
			sbdirty();
		}
	}
	if (nclean != lfs_sb_getnclean(fs)) {
		pwarn("NCLEAN GIVEN AS %d, SHOULD BE %d\n", lfs_sb_getnclean(fs),
		    nclean);
		if (preen || reply("FIX")) {
			lfs_sb_setnclean(fs, nclean);
			sbdirty();
		}
	}

	labelskew = 0;
	if (lfs_sb_getversion(fs) > 1 &&
	    lfs_sb_gets0addr(fs) < lfs_btofsb(fs, LFS_LABELPAD))
		labelskew = lfs_btofsb(fs, LFS_LABELPAD);
	bfree_ub = lfs_sb_getdsize(fs) - bb - labelskew;
	bfree_lb = lfs_sb_getdsize(fs) - ubb - labelskew;
	bfree_observed = (daddr_t)lfs_sb_getbfree(fs);
	if (bfree_observed < bfree_lb || bfree_observed > bfree_ub) {
		pwarn("BFREE GIVEN AS %jd, SHOULD BE BETWEEN %jd AND %jd\n",
		      (intmax_t)bfree_observed,
		      (intmax_t)bfree_lb,
		      (intmax_t)bfree_ub);
		if (preen || reply("FIX")) {
			lfs_sb_setbfree(fs, (bfree_lb + bfree_ub) / 2);
			sbdirty();
		}
	}
}
