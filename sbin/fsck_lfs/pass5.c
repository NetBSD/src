/* $NetBSD: pass5.c,v 1.10 2002/05/23 04:05:11 perseant Exp $	 */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <string.h>
#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern SEGUSE  *seg_table;

void
pass5()
{
	SEGUSE         *su;
	struct bufarea *bp;
	int             i;
	unsigned long   bb; /* total number of used blocks (lower bound) */
	unsigned long   ubb; /* upper bound number of used blocks */
	unsigned long   avail; /* blocks available for writing */
	unsigned long	dmeta; /* blocks in segsums and inodes */
	int             nclean; /* clean segments */
	size_t          labelskew;

	/*
	 * Check segment holdings against actual holdings.  Check for
	 * "clean" segments that contain live data.
	 */
	nclean = 0;
	avail = 0;
	bb = ubb = 0;
	dmeta = 0;
	for (i = 0; i < sblock.lfs_nseg; i++) {
		su = lfs_gseguse(i, &bp);
		if (!(su->su_flags & SEGUSE_DIRTY) &&
		    seg_table[i].su_nbytes > 0) {
			pwarn("%d bytes contained in 'clean' segment %d\n",
			      seg_table[i].su_nbytes, i);
			if (preen || reply("fix")) {
				su->su_flags |= SEGUSE_DIRTY;
				dirty(bp);
			}
		}
		if ((su->su_flags & SEGUSE_DIRTY) &&
		    su->su_nbytes != seg_table[i].su_nbytes) {
			pwarn("segment %d claims %d bytes but has %d",
			      i, su->su_nbytes, seg_table[i].su_nbytes);
			if (su->su_nbytes > seg_table[i].su_nbytes)
				pwarn(" (high by %d)\n", su->su_nbytes -
				      seg_table[i].su_nbytes);
			else
				pwarn(" (low by %d)\n", - su->su_nbytes +
				      seg_table[i].su_nbytes);
			if (preen || reply("fix")) {
				su->su_nbytes = seg_table[i].su_nbytes;
				dirty(bp);
			}
		}
		if (su->su_flags & SEGUSE_DIRTY) {
			bb += btofsb(&sblock, su->su_nbytes +
				    su->su_nsums * sblock.lfs_sumsize);
			ubb += btofsb(&sblock, su->su_nbytes +
				     su->su_nsums * sblock.lfs_sumsize +
				     su->su_ninos * sblock.lfs_ibsize);
			dmeta += btofsb(&sblock, 
				sblock.lfs_sumsize * su->su_nsums);
			dmeta += btofsb(&sblock, 
				sblock.lfs_ibsize * su->su_ninos);
		} else {
			nclean++;
			avail += segtod(&sblock, 1);
			if (su->su_flags & SEGUSE_SUPERBLOCK)
				avail -= btofsb(&sblock, LFS_SBPAD);
			if (i == 0 && sblock.lfs_version > 1 &&
			    sblock.lfs_start < btofsb(&sblock, LFS_LABELPAD))
				avail -= btofsb(&sblock, LFS_LABELPAD) - 
					sblock.lfs_start;
		}
		bp->b_flags &= ~B_INUSE;
	}
	/* Also may be available bytes in current seg */
	i = dtosn(&sblock, sblock.lfs_offset);
	avail += sntod(&sblock, i + 1) - sblock.lfs_offset;
	/* But do not count minfreesegs */
	avail -= segtod(&sblock, (sblock.lfs_minfreeseg -
				   (sblock.lfs_minfreeseg / 2)));

	if (dmeta != sblock.lfs_dmeta) { 
		pwarn("dmeta given as %d, should be %ld\n", sblock.lfs_dmeta, 
			dmeta); 
		if (preen || reply("fix")) { 
			sblock.lfs_dmeta = dmeta; 
			sbdirty(); 
		} 
	}
	if (avail != sblock.lfs_avail) {
		pwarn("avail given as %d, should be %ld\n", sblock.lfs_avail,
		      avail);
		if (preen || reply("fix")) {
			sblock.lfs_avail = avail;
			sbdirty();
		}
	}
	if (nclean != sblock.lfs_nclean) {
		pwarn("nclean given as %d, should be %d\n", sblock.lfs_nclean,
		      nclean);
		if (preen || reply("fix")) {
			sblock.lfs_nclean = nclean;
			sbdirty();
		}
	}
	labelskew = (sblock.lfs_version == 1 ? 0 : 
		btofsb(&sblock, LFS_LABELPAD));
	if (sblock.lfs_bfree > sblock.lfs_dsize - bb - labelskew ||
	    sblock.lfs_bfree < sblock.lfs_dsize - ubb - labelskew) {
		pwarn("bfree given as %d, should be between %ld and %ld\n",
		      sblock.lfs_bfree, sblock.lfs_dsize - ubb - labelskew,
		      sblock.lfs_dsize - bb - labelskew);
		if (preen || reply("fix")) {
			sblock.lfs_bfree = sblock.lfs_dsize - labelskew -
				(ubb + bb) / 2;
			sbdirty();
		}
	}
}
