/* $NetBSD: pass0.c,v 1.15 2003/03/31 19:56:59 perseant Exp $	 */

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
pass0()
{
	daddr_t daddr;
	CLEANERINFO *cip;
	IFILE *ifp;
	struct ubuf *bp;
	ino_t ino, plastino, nextino, *visited;
	int writeit = 0;

	/*
         * Check the inode free list for inuse inodes, and cycles.
	 * Make sure that all free inodes are in fact on the list.
         */
	visited = (ino_t *) malloc(maxino * sizeof(ino_t));
	memset(visited, 0, maxino * sizeof(ino_t));

	plastino = 0;
	ino = fs->lfs_freehd;
	while (ino) {
		if (ino >= maxino) {
			printf("! Ino %d out of range (last was %d)\n", ino,
			    plastino);
			break;
		}
		if (visited[ino]) {
			pwarn("! Ino %d already found on the free list!\n",
			    ino);
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
			pwarn("! Ino %d with daddr 0x%llx is on the free list!\n",
			    ino, (long long) daddr);
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
		pwarn("! Ino %d free, but not on the free list\n", ino);
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
		pwarn("! Free list tail should be %d (was %d)\n", plastino,
			cip->free_tail);
		if (preen || reply("FIX")) {
			cip->free_tail = plastino;
			writeit = 1;
		}
	}
	LFS_SYNC_CLEANERINFO(cip, fs, bp, writeit);
}
