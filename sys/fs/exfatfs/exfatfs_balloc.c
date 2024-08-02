/*	$NetBSD: exfatfs_balloc.c,v 1.1.2.4 2024/08/02 00:16:55 perseant Exp $	*/

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exfatfs_balloc.c,v 1.1.2.4 2024/08/02 00:16:55 perseant Exp $");

#include <sys/types.h>
#include <sys/buf.h>
#ifdef _KERNEL
# include <sys/vnode.h>
#else
# include <assert.h>
# include "vnode.h"
# include "bufcache.h"
typedef struct uvnode uvnode_t;
# define vnode uvnode
# define vnode_t uvnode_t
# define buf ubuf
# define bdwrite bwrite
#endif

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_balloc.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_mount.h>

#ifndef _KERNEL
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
#endif

/* #define EXFATFS_BALLOC_DEBUG */

#ifdef EXFATFS_BALLOC_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x) __nothing
#endif

/*
 * Routines to create and manage a compressed cluster bitmap.
 */

extern struct pool exfatfs_bitmap_pool;

/*
 * Convert a cluster ID into a disk address
 */
extern u_int64_t exfatfs_lookup_bitmap_blk(uint32_t);

/* How many bits are set in a byte of a given value */
static uint8_t byte2bitcount[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

/*
 * Find the next free cluster after "start" and allocate it.
 */
int
exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp)
{
	daddr_t lbn, blkno;
	uint32_t r, ostart, c;
	struct buf *bp;
	int off, error;
	uint8_t *data;
	uint32_t end;
	
	if (start < 2)
		start = 2;
	end = fs->xf_ClusterCount + 2;
	
	DPRINTF(("basic allocate cluster start=%u\n", (unsigned)start));
	
	ostart = start;
	c = 0;
	do {
		lbn = BITMAPLBN(fs, start);
		off = BITMAPOFF(fs, start);
		DPRINTF((" LBNOFF2CLUSTER(%u,%u)=%u, end=%u\n",
			 (unsigned)lbn, (unsigned)off,
			 (unsigned)LBNOFF2CLUSTER(fs, lbn, off),
			 (unsigned)end));
		while (LBNOFF2CLUSTER(fs, lbn, off) < end) {
			exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL, &blkno,
					    NULL);
			DPRINTF((" lbn %u -> bn 0x%x\n",
				 (unsigned)lbn, (unsigned)blkno));
			if ((error = bread(fs->xf_devvp, blkno, EXFATFS_LSIZE(fs), 0,
					   &bp)) != 0)
				return error;
			DPRINTF((" search %u..%u\n",
				(unsigned)off,
				(unsigned)(1 << BITMAPSHIFT(fs))));
			while (off < (1 << BITMAPSHIFT(fs))
			       && LBNOFF2CLUSTER(fs, lbn, off) < end) {
				data = (uint8_t *)bp->b_data;
				if (isclr(data, off)) {
					r = LBNOFF2CLUSTER(fs, lbn, off);
					break;
				}
				++off;
			}
			if (r != INVALID) {
				assert(r >= 2 && r < fs->xf_ClusterCount + 2);
				DPRINTF(("basic allocate cluster %u/%u"
					 " at lbn %u bit %d\n",
					 (unsigned)r, (unsigned)end,
					 (unsigned)lbn, (int)off));
				setbit(data, off);
				bdwrite(bp);
#ifdef _KERNEL
				mutex_enter(&fs->xf_lock);
#endif /* _KERNEL */
				--fs->xf_FreeClusterCount;
#ifdef _KERNEL
				mutex_exit(&fs->xf_lock);
#endif /* _KERNEL */
				if (cp != NULL)
					*cp = r;
				return 0;
			} else
				brelse(bp, 0);
			++lbn;
			off = 0;
			assert(++c <= 2 * fs->xf_ClusterCount);
		}

		/* If we've searched everywhere, we are done */
		if (start != ostart || ostart <= 2)
			break;

		/* We've only searched the end.  Now search the begninning. */
		end = start;
		start = 2;
		DPRINTF((" not found, now start=%u end=%u\n",
			 (unsigned)start, (unsigned)end));
	} while (1);
	
	DPRINTF(("no clusters to allocate, returning ENOSPC\n"));
	return ENOSPC;
}

int
exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno, int len)
{
	daddr_t lbn, blkno;
	struct buf *bp = NULL;
	int off, error, lbsize;
	uint8_t *data;

	lbn = BITMAPLBN(fs, cno);
	off = BITMAPOFF(fs, cno);
	lbsize = EXFATFS_LSIZE(fs) * NBBY;
	while (--len >= 0) {
		if (off >= lbsize) {
			/* Switch to a new block */
			off = 0;
			++lbn;
			if (bp != NULL)
				bdwrite(bp);
			bp = NULL;
		}
		if (bp == NULL) {
			exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL, &blkno, NULL);
			if ((error = bread(fs->xf_devvp, blkno, EXFATFS_LSIZE(fs), 0, &bp)) != 0)
				return error;
			data = (uint8_t *)bp->b_data;
		}
		DPRINTF(("basic deallocate cluster %u at lbn %u bit %d\n",
		 	(unsigned)cno, (unsigned)lbn, (int)off));
		assert(isset(data, off));
		clrbit(data, off);
		++off;
	}
	if (bp != NULL)
		bdwrite(bp);
#ifdef _KERNEL
	mutex_enter(&fs->xf_lock);
#endif /* _KERNEL */
	++fs->xf_FreeClusterCount;
#ifdef _KERNEL
	mutex_exit(&fs->xf_lock);
#endif /* _KERNEL */
	return 0;
}

int
exfatfs_bitmap_init(struct exfatfs *fs)
{
	daddr_t lbn, blkno;
	uint32_t cn, end;
	struct buf *bp;
	int i, off;
	uint8_t *data = NULL;
	
	/* Scan the bitmap to discover how many clusters are free */
	fs->xf_FreeClusterCount = fs->xf_ClusterCount;

	end = fs->xf_ClusterCount + 2;
	cn = 2;
	bp = NULL;
	while (cn < end) {
		if (BITMAPOFF(fs, cn) == 0) {
			if (bp != NULL)
				brelse(bp, 0);
			lbn = BITMAPLBN(fs, cn);
			exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL,
					    &blkno, NULL);
			bread(fs->xf_devvp, blkno, EXFATFS_LSIZE(fs), 0, &bp);
			data = (uint8_t *)bp->b_data;
		}
		for (off = 0; off < EXFATFS_LSIZE(fs) && cn < end; ++off) {
			if (cn >= end - NBBY) {
				for (i = 0; i < NBBY && cn < end; i++) {
					if (data[off] & (1 << i))
						--fs->xf_FreeClusterCount;
					++cn;
				}
			} else {
				fs->xf_FreeClusterCount
					-= byte2bitcount[data[off]];
				cn += NBBY;
			}
		}
	}
	if (bp)
		brelse(bp, 0);

	return 0;
}

void
exfatfs_bitmap_destroy(struct exfatfs *fs)
{
}
