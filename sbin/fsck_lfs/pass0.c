/* $NetBSD: pass0.c,v 1.32 2013/01/22 09:39:12 dholland Exp $	 */

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

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

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
	struct ubuf *bp, *cbp;
	ino_t ino, plastino, nextino, lowfreeino, freehd;
	char *visited;
	int writeit = 0;

	/*
	 * Check the inode free list for inuse inodes, and cycles.
	 * Make sure that all free inodes are in fact on the list.
	 */
	visited = ecalloc(maxino, sizeof(*visited));
	plastino = 0;
	lowfreeino = maxino;
	LFS_CLEANERINFO(cip, fs, cbp);
	freehd = ino = cip->free_head;
	brelse(cbp, 0);

	while (ino) {
		if (lowfreeino > ino)
			lowfreeino = ino;
		if (ino >= maxino) {
			pwarn("OUT OF RANGE INO %llu ON FREE LIST\n",
			    (unsigned long long)ino);
			break;
		}
		if (visited[ino]) {
			pwarn("INO %llu ALREADY FOUND ON FREE LIST\n",
			    (unsigned long long)ino);
			if (preen || reply("FIX") == 1) {
				/* plastino can't be zero */
				LFS_IENTRY(ifp, fs, plastino, bp);
				ifp->if_nextfree = 0;
				VOP_BWRITE(bp);
			}
			break;
		}
		visited[ino] = 1;
		LFS_IENTRY(ifp, fs, ino, bp);
		nextino = ifp->if_nextfree;
		daddr = ifp->if_daddr;
		brelse(bp, 0);
		if (daddr) {
			pwarn("INO %llu WITH DADDR 0x%llx ON FREE LIST\n",
			    (unsigned long long)ino, (long long) daddr);
			if (preen || reply("FIX") == 1) {
				if (plastino == 0) {
					freehd = nextino;
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
	for (ino = maxino - 1; ino > UFS_ROOTINO; --ino) {
		if (visited[ino])
			continue;

		LFS_IENTRY(ifp, fs, ino, bp);
		if (ifp->if_daddr) {
			brelse(bp, 0);
			continue;
		}
		pwarn("INO %llu FREE BUT NOT ON FREE LIST\n",
		    (unsigned long long)ino);
		if (preen || reply("FIX") == 1) {
			assert(ino != freehd);
			ifp->if_nextfree = freehd;
			VOP_BWRITE(bp);

			freehd = ino;
			sbdirty();

			/* If freelist was empty, this is the tail */
			if (plastino == 0)
				plastino = ino;
		} else
			brelse(bp, 0);
	}

	LFS_CLEANERINFO(cip, fs, cbp);
	if (cip->free_head != freehd) {
		/* They've already given us permission for this change */
		cip->free_head = freehd;
		writeit = 1;
	}
	if (freehd != fs->lfs_freehd) {
		pwarn("FREE LIST HEAD IN SUPERBLOCK SHOULD BE %d (WAS %d)\n",
			(int)fs->lfs_freehd, (int)freehd);
		if (preen || reply("FIX")) {
			fs->lfs_freehd = freehd;
			sbdirty();
		}
	}
	if (cip->free_tail != plastino) {
		pwarn("FREE LIST TAIL SHOULD BE %llu (WAS %llu)\n",
		    (unsigned long long)plastino,
		    (unsigned long long)cip->free_tail);
		if (preen || reply("FIX")) {
			cip->free_tail = plastino;
			writeit = 1;
		}
	}

	if (writeit)
		LFS_SYNC_CLEANERINFO(cip, fs, cbp, writeit);
	else
		brelse(cbp, 0);

	if (fs->lfs_freehd == 0) {
		pwarn("%sree list head is 0x0\n", preen ? "f" : "F");
		if (preen || reply("FIX"))
			extend_ifile(fs);
	}
}
