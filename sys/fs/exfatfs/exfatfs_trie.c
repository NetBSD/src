/*	$NetBSD: exfatfs_trie.c,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: exfatfs_trie.c,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $");

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
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_mount.h>
#include <fs/exfatfs/exfatfs_trie.h>

#ifndef _KERNEL
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# define pool_get(a, b) (struct xf_bitmap_node *)malloc(sizeof(struct xf_bitmap_node))
# define pool_put(a, b) free((b))
#endif

/* #define TEST_TRIE */
#define USE_BASIC

/* #define EXFATFS_TRIE_DEBUG */

#ifdef EXFATFS_TRIE_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x)
#endif

/*
 * Routines to create and manage a compressed cluster bitmap.
 */

extern struct pool exfatfs_bitmap_pool;

static int exfatfs_bitmap_alloc_basic(struct exfatfs *fs, uint32_t start, uint32_t end, uint32_t *cp, int alloc);
#ifndef USE_BASIC
static int exfatfs_bitmap_isalloc_basic(struct exfatfs *fs, uint32_t cno, int *tfp);
#endif /* USE_BASIC */


/*
 * Convert a cluster ID into a disk address
 */
extern u_int64_t exfatfs_lookup_bitmap_blk(uint32_t);

#ifndef USE_BASIC
static void exfatfs_free_bitmap_recurse(struct xf_bitmap_node *bnp)
{
	int i;
	
	for (i = 0; i < TBN_CHILDREN_COUNT; i++)
		if (bnp->children[i] != NULL)
			exfatfs_free_bitmap_recurse(bnp->children[i]);
	pool_put(&exfatfs_bitmap_pool, bnp);
}

/*
 * Allocate or deallocate a single cluster.
 */
static int
exfatfs_bitmap_adj(struct exfatfs *fs,
		   struct xf_bitmap_node *bnp,
		   uint32_t cno, int level, int dir, int alloc)
{
	uint32_t this_mask, next_mask, rcno;
	int i, error;

	DPRINTF(("exfatfs_bitmap_adj(%p, %p, %u, %d, %d, %d)\n",
		 fs, bnp, cno, level, dir, alloc));
	
	exfatfs_check_fence(fs);

	/* If we are full and deallocating, fill children */
	if (level > BOTTOM_LEVEL && TBN_FULL(bnp, level) && dir < 0) {
		for (i = 0; i < TBN_CHILDREN_COUNT; i++) {
			if (bnp->children[i] != NULL)
				continue;
			bnp->children[i] = pool_get(&exfatfs_bitmap_pool,
						    PR_WAITOK);
			memset(bnp->children[i], 0, sizeof(*bnp));
			bnp->children[i]->count = TBN_SIZE(level - 1);
		}
	}
	
	bnp->count += dir;
	if (level == BOTTOM_LEVEL) {
		error = exfatfs_bitmap_alloc_basic(fs, cno, cno + 1,
						   NULL, alloc);
		if (error)
			return error;
	}

	/* If we are now full or empty, delete children. */
	if (TBN_EMPTY(bnp, level) || TBN_FULL(bnp, level)) {
		DPRINTF((" now full/empty\n"));
		for (i = 0; i < TBN_CHILDREN_COUNT; i++) {
			if (bnp->children[i] != NULL)
				pool_put(&exfatfs_bitmap_pool,
					 bnp->children[i]);
			bnp->children[i] = NULL;
		}
		exfatfs_check_fence(fs);
		return 0;
	}

	if (level == BOTTOM_LEVEL)
		return 0;

	/*
	 * Otherwise, mark our (de)allocation in the appropriate child,
	 * allocating if necessary.
	 */
	this_mask = ((1 << (level * TBN_CHILDREN_SHIFT)) - 1);
	next_mask = ((1 << ((level - 1) * TBN_CHILDREN_SHIFT)) - 1);
	rcno = ((cno & this_mask) & ~next_mask)
		>> ((level - 1) * TBN_CHILDREN_SHIFT);

	if (bnp->children[rcno] == NULL) {
		bnp->children[rcno] = pool_get(&exfatfs_bitmap_pool, PR_WAITOK);
		memset(bnp->children[rcno], 0, sizeof(*bnp));
	}

	/* Drill down */
	assert(rcno < TBN_CHILDREN_COUNT);
	return exfatfs_bitmap_adj(fs, bnp->children[rcno], cno, --level,
				  dir, alloc);
}
#endif /* ! USE_BASIC */

/*
 * Find the next free cluster after "start" and optionally allocate it.
 */
static int
exfatfs_bitmap_scan(struct exfatfs *fs,
		    struct xf_bitmap_node *bnp,
		    uint32_t start, unsigned level,
		    uint32_t *cp,
		    int alloc)
{
	uint32_t r = INVALID;
	uint32_t this_mask = ((1 << (level * TBN_CHILDREN_SHIFT)) - 1);
	uint32_t next_mask = ((1 << ((level - 1) * TBN_CHILDREN_SHIFT)) - 1);
	uint32_t base = start & ~this_mask;
	uint32_t rcno = ((start & this_mask) & ~next_mask)
		>> ((level - 1) * TBN_CHILDREN_SHIFT);
	unsigned i;
	int error;

	*cp = INVALID;
	
	exfatfs_check_fence(fs);
	if (level == BOTTOM_LEVEL) {
		uint32_t end = base + TBN_SIZE(level);
		return exfatfs_bitmap_alloc_basic(fs, start, end, cp, alloc);
	}

	/*
	 * Not bottom level.  Look for space in the lower levels.
	 */
	
	if (TBN_FULL(bnp, level))
		return ENOSPC;

	for (i = rcno; i < TBN_CHILDREN_COUNT; ++i) {
		if (bnp->children[i] == NULL) {
			/* We must be empty */
			if (bnp->count == 0) {
				bnp->children[i] = pool_get(&exfatfs_bitmap_pool, PR_WAITOK);
				memset(bnp->children[i], 0, sizeof(*bnp));
			} else
				continue;
		}

		/* Our child has some space; drill down. */
		error = exfatfs_bitmap_scan(fs, bnp->children[i], start,
					    --level, cp, alloc);
		if (error == 0 && r != INVALID) {
			exfatfs_check_fence(fs);
			return 0;
		}
	}

	/* Not reached? */
	exfatfs_check_fence(fs);
	return ENOSPC;
}

/*
 * Find a large chunk and allocate the beginning of it.
 * Use a greedy strategy walking down the trie.
 * We use this to allocate the first block in a file,
 * in the hopes that subsequent allocations are more
 * likely to be contiguous.
 */
/* static */ int
exfatfs_bitmap_alloc_greedy(struct exfatfs *fs,
			    struct xf_bitmap_node *bnp,
			    uint32_t start, int level, uint32_t *cp,
			    int alloc);
/* static */ int
exfatfs_bitmap_alloc_greedy(struct exfatfs *fs,
			    struct xf_bitmap_node *bnp,
			    uint32_t start, int level, uint32_t *cp,
			    int alloc)
{
	unsigned i;
	int min1, maxchild, runchild;
	unsigned min1count, maxcount, runcount;
	
	if (level == BOTTOM_LEVEL) {
		/*
		 * Scan the block for the largest contiguous
		 * free area.  Allocate the middle of this area.
		 */
		/* XXX */
		return exfatfs_bitmap_alloc_basic(fs, start,
						  start + TBN_BLOCK_SIZE,
						  cp, alloc);
	} else {
		/*
		 * Scan the children to find the largest
		 * contiguous group of emptry children.
		 * If no children are empty, drill down
		 * into the most empty.
		 */
		min1 = maxchild = runchild = -1;
		min1count = maxcount = runcount = 0;
		for (i = 0; i < TBN_CHILDREN_COUNT; i++) {
			if (bnp->children[i] == NULL)
				continue;
			if (min1 < 0 || min1count > bnp->children[i]->count) {
				min1 = i;
				min1count = bnp->children[i]->count;
			}
			if (bnp->children[i]->count == 0) {
				if (runchild < 0) {
					runchild = i;
					runcount = 0;
				}
				runcount += TBN_CHILDREN_COUNT;
				if (runcount > maxcount) {
					maxchild = runchild;
					maxcount = runcount;
				}
			}
		}
		if (maxcount > 0) {
			int midmax = maxchild + maxcount / 2;
			/* Drill into the center of this range */
			return exfatfs_bitmap_scan(fs, bnp->children[midmax],
						   0, level - 1, cp, alloc);
		} else {
			/* Drill down into the most empty child */
			return exfatfs_bitmap_alloc_greedy(fs,
						     bnp->children[min1],
						     0, level - 1, cp, alloc);
		}
	}
	return 0;
}

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

#ifndef USE_BASIC
static int
exfatfs_init_trie(struct exfatfs *fs, int doread)
{
	int i, j, byte, bitcount, error;
	struct buf *bp;
	uint32_t cno;

	exfatfs_check_fence(fs);
	
	if (fs->xf_bitmapvp == NULL)
	    return EBADF;
	
	fs->xf_bitmap_trie = pool_get(&exfatfs_bitmap_pool, PR_WAITOK);
	memset(fs->xf_bitmap_trie, 0, sizeof(*fs->xf_bitmap_trie));

	/* Discover which is the top level */
	for (i = BOTTOM_LEVEL; ; i++) {
		DPRINTF(("TRIE: level %d gives %llu vs %llu\n", i,
		       (unsigned long long)fs->xf_ClusterCount,
			 (unsigned long long)(1 << (i * TBN_CHILDREN_SHIFT)) - 1));
		if (fs->xf_ClusterCount < (1 << (i * TBN_CHILDREN_SHIFT)) - 1) {
			fs->xf_bitmap_toplevel = i;
			break;
		}
	}
	DPRINTF(("TRIE: top level is %d\n", fs->xf_bitmap_toplevel));

	if (!doread) {
		exfatfs_check_fence(fs);
		return 0;
	}

	/* Populate from disk */
	for (i = 0; i < GET_DSE_VALIDDATALENGTH(VTOXI(fs->xf_bitmapvp));
	     i += SECSIZE(fs)) {
		if ((error = bread(fs->xf_bitmapvp, i >> fs->xf_BytesPerSectorShift,
				   SECSIZE(fs), 0, &bp)) != 0)
			return error;
		DPRINTF(("TRIE: read lblkno %lu blkno 0x%lx\n",
		       (unsigned long)bp->b_lblkno,
			 (unsigned long)bp->b_blkno));
		for (j = 0; j < SECSIZE(fs); j++) {
			cno = (i + j) * NBBY;
			if (cno > fs->xf_ClusterCount)
				break;
			byte = ((char *)bp->b_data)[j];
			if (cno + NBBY > fs->xf_ClusterCount)
				byte &= ((1 << (fs->xf_ClusterCount & (NBBY - 1))) - 1);
			bitcount = byte2bitcount[byte];
			DPRINTF(("TRIE: cluster %lu..%lu, %d busy\n",
			       (unsigned long)(cno),
				 (unsigned long)(cno + NBBY - 1), bitcount));
			exfatfs_bitmap_adj(fs, fs->xf_bitmap_trie, cno,
				  fs->xf_bitmap_toplevel, bitcount, 0);
		}
		brelse(bp, 0);
	}
	exfatfs_check_fence(fs);
	return 0;
}

static int
exfatfs_bitmap_isalloc_trie(struct exfatfs *fs,
			    struct xf_bitmap_node *bnp,
			    uint32_t cno, int level, int *tfp)
{
	uint32_t this_mask, next_mask, rcno;

	if (level == BOTTOM_LEVEL)
		return exfatfs_bitmap_isalloc_basic(fs, cno, tfp);

	/* If we are full or empty, we don't need to drill further */
	if (TBN_EMPTY(bnp, level)) {
		*tfp = 0;
		return 0;
	}

	if (TBN_FULL(bnp, level)) {
		*tfp = 1;
		return 0;
	}

	/*
	 * Otherwise, drill down
	 */
	this_mask = ((1 << (level * TBN_CHILDREN_SHIFT)) - 1);
	next_mask = ((1 << ((level - 1) * TBN_CHILDREN_SHIFT)) - 1);
	rcno = ((cno & this_mask) & ~next_mask)
		>> ((level - 1) * TBN_CHILDREN_SHIFT);
	assert(rcno < TBN_CHILDREN_COUNT);
	return exfatfs_bitmap_isalloc_trie(fs, bnp->children[rcno],
					   cno, --level, tfp);
}

static int
exfatfs_bitmap_alloc_trie(struct exfatfs *fs, uint32_t start, uint32_t *cp)
{
	return exfatfs_bitmap_scan(fs, fs->xf_bitmap_trie, start,
				   fs->xf_bitmap_toplevel, cp, 1);
}

static int
exfatfs_bitmap_dealloc_trie(struct exfatfs *fs, uint32_t cno)
{
	return exfatfs_bitmap_adj(fs, fs->xf_bitmap_trie, cno,
				  fs->xf_bitmap_toplevel, -1, 1);
}

static int
exfatfs_bitmap_init_trie(struct exfatfs *fs, int doread)
{
	return exfatfs_init_trie(fs, doread);
}

static void
exfatfs_bitmap_destroy_trie(struct exfatfs *fs)
{
	exfatfs_free_bitmap_recurse(fs->xf_bitmap_trie);
}
#endif /* USE_BASIC */

/*
 * A basic version that just reads values from the bitmap on disk.
 * The TRIE version uses this when it reaches a low enough level.
 */

static int
exfatfs_bitmap_alloc_basic(struct exfatfs *fs, uint32_t start, uint32_t end, uint32_t *cp, int alloc)
{
	daddr_t lbn, blkno;
	uint32_t r, ostart, c;
	struct buf *bp;
	int off, error;
	uint8_t *data;
	
	if (start < 2)
		start = 2;
	if (end > fs->xf_ClusterCount + 2) /* e.g. (uint32_t)-1 */
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
			exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL, &blkno, NULL);
			DPRINTF((" lbn %u -> bn 0x%x\n",
				 (unsigned)lbn, (unsigned)blkno));
			if ((error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp)) != 0)
				return error;
			DPRINTF((" search %u..%u\n",
				 (unsigned)off, (unsigned)(1 << BITMAPSHIFT(fs))));
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
				assert(r >= 2 && r < fs->xf_ClusterCount - 2);
				DPRINTF(("basic allocate cluster %u/%u at lbn %u bit %d\n",
					 (unsigned)r, (unsigned)end, (unsigned)lbn, (int)off));
				setbit(data, off);
				bdwrite(bp);
				--fs->xf_FreeClusterCount;
				if (cp != NULL)
					*cp = r;
				return 0;
			} else
				brelse(bp, 0);
			++lbn;
			off = 0;
			assert(++c <= 2 * fs->xf_ClusterCount);
		}
		if (start == ostart && start > 2) {
			end = start;
			start = 2;
			DPRINTF((" not found, now start=%u end=%u\n",
				 (unsigned)start, (unsigned)end));
			continue;
		}
	} while (0);
	
	return ENOSPC;
}

static int
exfatfs_bitmap_dealloc_basic(struct exfatfs *fs, uint32_t cno)
{
	daddr_t lbn, blkno;
	struct buf *bp;
	int off, error;
	uint8_t *data;
	
	lbn = BITMAPLBN(fs, cno);
	off = BITMAPOFF(fs, cno);
	exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL, &blkno, NULL);
	if ((error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp)) != 0)
		return error;
	data = (uint8_t *)bp->b_data;
	DPRINTF(("basic deallocate cluster %u at lbn %u bit %d\n",
		 (unsigned)cno, (unsigned)lbn, (int)off));
	clrbit(data, off);
	bdwrite(bp);
	++fs->xf_FreeClusterCount;
	return 0;
}

#ifndef USE_BASIC
static int
exfatfs_bitmap_isalloc_basic(struct exfatfs *fs, uint32_t cno, int *tfp)
{
	daddr_t lbn, blkno;
	struct buf *bp;
	int off, error;
	uint8_t *data;
	
	lbn = BITMAPLBN(fs, cno);
	off = BITMAPOFF(fs, cno);
	exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL, &blkno, NULL);
	if ((error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp)) != 0)
		return error;
	data = (uint8_t *)bp->b_data;
	*tfp = isset(data, off);
	brelse(bp, 0);
	return 0;
}
#endif /* USE_BASIC */

static int
exfatfs_bitmap_init_basic(struct exfatfs *fs, int doread)
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
			bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp);
			data = (uint8_t *)bp->b_data;
		}
		for (off = 0; off < SECSIZE(fs) && cn < end; ++off) {
			if (cn >= end - NBBY) {
				for (i = 0; i < NBBY && cn < end; i++) {
					if (data[off] & (1 << i))
						--fs->xf_FreeClusterCount;
					++cn;
				}
			} else {
				fs->xf_FreeClusterCount -= byte2bitcount[data[off]];
				cn += NBBY;
			}
		}
	}
	if (bp)
		brelse(bp, 0);

	return 0;
}

static void
exfatfs_bitmap_destroy_basic(struct exfatfs *fs)
{
}

#ifdef TEST_TRIE
/*
 * Perform TRIE operations first, but without changing the on-disk data.
 * Assert the same results as the basic operations.
 */

int exfatfs_bitmap_init(struct exfatfs *fs, int i) {
	int error;
	
	error = exfatfs_bitmap_init_basic(fs, i);
	if (error)
		return error;
	error = exfatfs_bitmap_init_trie(fs, i);
	return error;
}

void exfatfs_bitmap_destroy(struct exfatfs *fs) {
	exfatfs_bitmap_destroy_trie(fs);
	exfatfs_bitmap_destroy_basic(fs);
}

int exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp)
{
	uint32_t cn_basic;
	uint32_t cn_trie;
	int error;

#ifdef _KERNEL
	vref(fs->xf_bitmapvp);
	vn_lock(fs->xf_bitmapvp, 0);
#endif /* _KERNEL */
	
	error = exfatfs_bitmap_alloc_trie(fs, start, &cn_trie);
	if (error)
		goto out;
	error = exfatfs_bitmap_alloc_basic(fs, start, -1, &cn_basic, 1);
	if (error)
		goto out;

	/* We should have allocated the same block */
	assert(cn_trie == cn_basic);

out:
#ifdef _KERNEL
	vput(fs->xf_bitmapvp);
#endif /* _KERNEL */

	*cp = cn_basic;
	return error;
}

int exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno) {
	int error;
	int tf_basic, tf_trie;

#ifdef _KERNEL
	vref(fs->xf_bitmapvp);
	vn_lock(fs->xf_bitmapvp, 0);
#endif /* _KERNEL */

	/*
	 * Check that the allocation status of the requested
	 * block is the same in both systems before deallocation
	 */
	error = exfatfs_bitmap_isalloc_trie(fs, fs->xf_bitmap_trie,
					    cno, fs->xf_bitmap_toplevel,
					    &tf_trie);
	if (error)
		goto out;
	error = exfatfs_bitmap_isalloc_basic(fs, cno, &tf_basic);
	if (error)
		goto out;
	assert(tf_trie == tf_basic);

	/* Actually deallocate the block */
	error = exfatfs_bitmap_dealloc_trie(fs, cno);
	if (error)
		goto out;
	error = exfatfs_bitmap_dealloc_basic(fs, cno);
	if (error)
		goto out;

	/* Check it is the same after dealloc too */
	error = exfatfs_bitmap_isalloc_trie(fs, fs->xf_bitmap_trie,
					    cno, fs->xf_bitmap_toplevel,
					    &tf_trie);
	if (error)
		goto out;
	error = exfatfs_bitmap_isalloc_basic(fs, cno, &tf_basic);
	if (error)
		goto out;
	assert(tf_trie == tf_basic);

out:
#ifdef _KERNEL
	vput(fs->xf_bitmapvp);
#endif /* _KERNEL */
	return error;
}
#elif defined USE_BASIC

/*
 * Just use the basic versions.
 */

int exfatfs_bitmap_init(struct exfatfs *fs, int i) {
	return exfatfs_bitmap_init_basic(fs, i);
}

void exfatfs_bitmap_destroy(struct exfatfs *fs) {
	exfatfs_bitmap_destroy_basic(fs);
}

int exfatfs_bitmap_alloc0(struct exfatfs *fs, uint32_t start, uint32_t *cp) {
	return exfatfs_bitmap_alloc_basic(fs, start, -1, cp, 1);
}

int exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp) {
	return exfatfs_bitmap_alloc_basic(fs, start, -1, cp, 1);
}

int exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno) {
	return exfatfs_bitmap_dealloc_basic(fs, cno);
}

#else /* ! TEST_TRIE && ! USE_BASIC */
/*
 * Just use the TRIE versions.
 */

int exfatfs_bitmap_init(struct exfatfs *fs, int i) {
	return exfatfs_bitmap_init_trie(fs, i);
}

void exfatfs_bitmap_destroy(struct exfatfs *fs) {
	exfatfs_bitmap_destroy_trie(fs);
}

int exfatfs_bitmap_alloc0(struct exfatfs *fs, uint32_t start, uint32_t *cp) {
	return exfatfs_bitmap_alloc_greedy(fs, fs->xf_bitmap_trie,
					   start, fs->xf_bitmap_toplevel,
					   cp, 1);
}

int exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp) {
	return exfatfs_bitmap_alloc_trie(fs, start, cp);
}

int exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno) {
	return exfatfs_bitmap_dealloc_trie(fs, cno);
}
#endif /* ! TEST_TRIE */

