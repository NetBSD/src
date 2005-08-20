/* $NetBSD: pass0.c,v 1.23 2005/08/20 14:59:20 kent Exp $	 */

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 The NetBSD Foundation, Inc.
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
/*-
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
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern int fake_cleanseg;

/*
 * Pass 0.  Check the LFS partial segments for valid checksums, correcting
 * if necessary.  Also check for valid offsets for inode and finfo blocks.
 */
/*
 * XXX more could be done here---consistency between inode-held blocks and
 * finfo blocks, for one thing.
 */

#define dbshift (fs->lfs_bshift - fs->lfs_blktodb)

void
pass0(void)
{
	daddr_t daddr;
	CLEANERINFO *cip;
	IFILE *ifp;
	struct ubuf *bp;
	ino_t ino, plastino, nextino, *visited, lowfreeino;
	int writeit = 0;
	int count;
	long long totaldist;

	/*
         * Check the inode free list for inuse inodes, and cycles.
	 * Make sure that all free inodes are in fact on the list.
         */
	visited = (ino_t *) malloc(maxino * sizeof(ino_t));
	memset(visited, 0, maxino * sizeof(ino_t));

#ifdef BAD
	/*
	 * Scramble the free list, to trigger the optimizer below.
	 * You don't want this unless you are debugging fsck_lfs itself.
	 */
	if (!preen && reply("SCRAMBLE FREE LIST") == 1) {
		ino_t topino, botino, tail;
		botino = 0;
		topino = maxino;
		while (botino < topino) {
			for (--topino; botino < topino; --topino) {
				LFS_IENTRY(ifp, fs, topino, bp);
				if (ifp->if_daddr == 0)
					break;
				brelse(bp);
				bp = NULL;
			}
			if (topino == botino)
				break;
			if (botino > 0) {
				ifp->if_nextfree = botino;
			} else {
				ifp->if_nextfree = 0x0;
				tail = topino;
			}
			VOP_BWRITE(bp);
			ino = topino;
		
			for (++botino; botino < topino; ++botino) {
				LFS_IENTRY(ifp, fs, botino, bp);
				if (ifp->if_daddr == 0)
					break;
				brelse(bp);
				bp = NULL;
			}
			if (topino == botino)
				break;
			ifp->if_nextfree = topino;
			VOP_BWRITE(bp);
			ino = botino;
		}
		LFS_CLEANERINFO(cip, fs, bp);
		cip->free_head = fs->lfs_freehd = ino;
		cip->free_tail = tail;
		LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);
	}
#endif /* BAD */

	count = 0;
	plastino = 0;
	totaldist = 0;
	lowfreeino = maxino;
	ino = fs->lfs_freehd;
	while (ino) {
		if (lowfreeino > ino)
			lowfreeino = ino;
		if (plastino > 0) {
			totaldist += abs(ino - plastino);
			++count;
		}
		if (ino >= maxino) {
			printf("! Ino %llu out of range (last was %llu)\n",
			    (unsigned long long)ino,
			    (unsigned long long)plastino);
			break;
		}
		if (visited[ino]) {
			pwarn("! Ino %llu already found on the free list!\n",
			    (unsigned long long)ino);
			if (preen || reply("FIX") == 1) {
				/* plastino can't be zero */
				LFS_IENTRY(ifp, fs, plastino, bp);
				ifp->if_nextfree = 0;
				VOP_BWRITE(bp);
			}
			break;
		}
		++visited[ino];
		LFS_IENTRY(ifp, fs, ino, bp);
		nextino = ifp->if_nextfree;
		daddr = ifp->if_daddr;
		brelse(bp);
		if (daddr) {
			pwarn("! Ino %llu with daddr 0x%llx is on the "
			    "free list!\n",
			    (unsigned long long)ino, (long long) daddr);
			if (preen || reply("FIX") == 1) {
				if (plastino == 0) {
					fs->lfs_freehd = nextino;
					sbdirty();
				} else {
					LFS_IENTRY(ifp, fs, plastino, bp);
					ifp->if_nextfree = nextino;
					VOP_BWRITE(bp);
				}
				ino = nextino;
				continue;
			}
		}
		plastino = ino;
		ino = nextino;
	}


	/*
	 * Make sure all free inodes were found on the list
	 */
	for (ino = ROOTINO + 1; ino < maxino; ++ino) {
		if (visited[ino])
			continue;

		LFS_IENTRY(ifp, fs, ino, bp);
		if (ifp->if_daddr) {
			brelse(bp);
			continue;
		}
		pwarn("! Ino %llu free, but not on the free list\n",
		    (unsigned long long)ino);
		if (preen || reply("FIX") == 1) {
			ifp->if_nextfree = fs->lfs_freehd;
			fs->lfs_freehd = ino;
			sbdirty();
			VOP_BWRITE(bp);
		} else
			brelse(bp);
	}

	LFS_CLEANERINFO(cip, fs, bp);
	if (cip->free_head != fs->lfs_freehd) {
		pwarn("! Free list head should be %d (was %d)\n",
			fs->lfs_freehd, cip->free_head);
		if (preen || reply("FIX")) {
			cip->free_head = fs->lfs_freehd;
			writeit = 1;
		}
	}
	if (cip->free_tail != plastino) {
		pwarn("! Free list tail should be %llu (was %d)\n",
		    (unsigned long long)plastino, cip->free_tail);
		if (preen || reply("FIX")) {
			cip->free_tail = plastino;
			writeit = 1;
		}
	}
	if (writeit)
		LFS_SYNC_CLEANERINFO(cip, fs, bp, writeit);
	else
		brelse(bp);

	if (fs->lfs_freehd == 0) {
		pwarn("%sree list head is 0x0\n", preen ? "f" : "F");
		if (preen || reply("FIX")) {
			extend_ifile(fs);
			reset_maxino(((VTOI(fs->lfs_ivnode)->i_ffs1_size >>
				       fs->lfs_bsize) -
				      fs->lfs_segtabsz - fs->lfs_cleansz) *
				     fs->lfs_ifpb);
		}
	}

	/*
	 * Check the distance between sequential free list entries.
	 * An ideally ordered free list will have the sum of these
	 * distances <= the number of inodes in the inode list.
	 * If the observed distance is too high, reorder the list.
	 * Strictly speaking, this is not an error, but it optimizes the
	 * speed in creation of files and should help a tiny bit with
	 * cleaner thrash as well.
	 */
	if (totaldist > 4 * maxino) {
		pwarn("%sotal inode list traversal length %" PRIu64 "x list length%s\n",
		      (preen ? "t" : "T"), totaldist/maxino,
		      (preen ? ", optimizing" : ""));
		if (preen || reply("OPTIMIZE") == 1) {
			plastino = lowfreeino;
			for (ino = lowfreeino + 1; ino < maxino; ino++) {
				LFS_IENTRY(ifp, fs, ino, bp);
				daddr = ifp->if_daddr;
				brelse(bp);
				if (daddr == 0) {
					LFS_IENTRY(ifp, fs, plastino, bp);
					ifp->if_nextfree = ino;
					VOP_BWRITE(bp);
					plastino = ino;
				}
			}
			LFS_CLEANERINFO(cip, fs, bp);
			cip->free_head = fs->lfs_freehd = lowfreeino;
			cip->free_tail = plastino;
			LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);
		}
	}
}
