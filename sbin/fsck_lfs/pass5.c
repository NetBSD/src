/* $NetBSD: pass5.c,v 1.22 2006/10/16 03:21:34 christos Exp $	 */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <string.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern SEGUSE *seg_table;
extern off_t locked_queue_bytes;

void
pass5(void)
{
	SEGUSE *su;
	struct ubuf *bp;
	int i;
	unsigned long bb;	/* total number of used blocks (lower bound) */
	unsigned long ubb;	/* upper bound number of used blocks */
	unsigned long avail;	/* blocks available for writing */
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
	for (i = 0; i < fs->lfs_nseg; i++) {
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
			bb += btofsb(fs, su->su_nbytes +
			    su->su_nsums * fs->lfs_sumsize);
			ubb += btofsb(fs, su->su_nbytes +
			    su->su_nsums * fs->lfs_sumsize +
			    su->su_ninos * fs->lfs_ibsize);
			dmeta += btofsb(fs,
			    fs->lfs_sumsize * su->su_nsums);
			dmeta += btofsb(fs,
			    fs->lfs_ibsize * su->su_ninos);
		} else {
			nclean++;
			avail += segtod(fs, 1);
			if (su->su_flags & SEGUSE_SUPERBLOCK)
				avail -= btofsb(fs, LFS_SBPAD);
			if (i == 0 && fs->lfs_version > 1 &&
			    fs->lfs_start < btofsb(fs, LFS_LABELPAD))
				avail -= btofsb(fs, LFS_LABELPAD) -
				    fs->lfs_start;
		}
		if (diddirty)
			VOP_BWRITE(bp);
		else
			brelse(bp);
	}

	/* Also may be available bytes in current seg */
	i = dtosn(fs, fs->lfs_offset);
	avail += sntod(fs, i + 1) - fs->lfs_offset;
	/* But do not count minfreesegs */
	avail -= segtod(fs, (fs->lfs_minfreeseg -
		(fs->lfs_minfreeseg / 2)));
	/* Note we may have bytes to write yet */
	avail -= btofsb(fs, locked_queue_bytes);

	if (idaddr)
		pwarn("NOTE: when using -i, expect discrepancies in dmeta,"
		      " avail, nclean, bfree\n");
	if (dmeta != fs->lfs_dmeta) {
		pwarn("DMETA GIVEN AS %d, SHOULD BE %ld\n", fs->lfs_dmeta,
		    dmeta);
		if (preen || reply("FIX")) {
			fs->lfs_dmeta = dmeta;
			sbdirty();
		}
	}
	if (avail != fs->lfs_avail) {
		pwarn("AVAIL GIVEN AS %d, SHOULD BE %ld\n", fs->lfs_avail,
		    avail);
		if (preen || reply("FIX")) {
			fs->lfs_avail = avail;
			sbdirty();
		}
	}
	if (nclean != fs->lfs_nclean) {
		pwarn("NCLEAN GIVEN AS %d, SHOULD BE %d\n", fs->lfs_nclean,
		    nclean);
		if (preen || reply("FIX")) {
			fs->lfs_nclean = nclean;
			sbdirty();
		}
	}

	labelskew = 0;
	if (fs->lfs_version > 1 &&
	    fs->lfs_start < btofsb(fs, LFS_LABELPAD))
		labelskew = btofsb(fs, LFS_LABELPAD);
	if (fs->lfs_bfree > fs->lfs_dsize - bb - labelskew ||
	    fs->lfs_bfree < fs->lfs_dsize - ubb - labelskew) {
		pwarn("BFREE GIVEN AS %d, SHOULD BE BETWEEN %ld AND %ld\n",
		    fs->lfs_bfree, (fs->lfs_dsize - ubb - labelskew),
		    fs->lfs_dsize - bb - labelskew);
		if (preen || reply("FIX")) {
			fs->lfs_bfree =
				((fs->lfs_dsize - labelskew - ubb) +
				 fs->lfs_dsize - labelskew - bb) / 2;
			sbdirty();
		}
	}
}
