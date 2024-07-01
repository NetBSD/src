/*	$NetBSD: exfatfs_trie_basic.c,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: exfatfs_trie_basic.c,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $");

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
# define pool_get(a, b) (struct xf_bitmap_node *)			\
			malloc(sizeof(struct xf_bitmap_node))
# define pool_put(a, b) free((b))
#endif

/* #define EXFATFS_TRIE_DEBUG */

#ifdef EXFATFS_TRIE_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x) __nothing
#endif

/*
 * Routines to create and manage a compressed cluster bitmap.
 */

extern struct pool exfatfs_bitmap_pool;

#define BN_FULL(bp, level) ((bp)->count == (1 << (level * BN_CHILDREN_SHIFT)) \
									- 1)

/* Convert cluster number to disk address and offset */
#define NBBYSHIFT 3 /* 1 << NBBYSHIFT == NBBY == 8 */
#define BITMAPSHIFT(fs)  ((fs)->xf_BytesPerSectorShift + NBBYSHIFT)
#define BITMAPLBN(fs, cn) (((cn) - 2) >> BITMAPSHIFT(fs))
#define BITMAPOFF(fs, cn) (((cn) - 2) & ((1 << BITMAPSHIFT(fs)) - 1))
#define LBNOFF2CLUSTER(fs, lbn, off) ((lbn << BITMAPSHIFT(fs)) + (off) + 2)

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

#ifdef EXFATFS_USE_TRIE
static void exfatfs_free_bitmap_recurse(struct xf_bitmap_node *bnp)
{
	int i;
	
	for (i = 0; i < BN_CHILDREN_COUNT; i++)
		if (bnp->children[i] != NULL)
			exfatfs_free_bitmap_recurse(bnp->children[i]);
	pool_put(&exfatfs_bitmap_pool, bnp);
}

static int
exfatfs_bitmap_adj(struct exfatfs *fs,
		   struct xf_bitmap_node *bnp,
		   uint32_t cno, int level, int dir, int alloc)
{
	int i;
	struct buf *bp;
	uint8_t *data;
	uint32_t this_mask = ((1 << (level * BN_CHILDREN_SHIFT)) - 1);
	uint32_t next_mask = ((1 << ((level - 1) * BN_CHILDREN_SHIFT)) - 1);
	uint32_t remainder = cno & this_mask;
	uint32_t rcno = ((cno & this_mask) & ~next_mask)
		>> ((level - 1) * BN_CHILDREN_SHIFT);
	daddr_t secno;
	int error;

	exfatfs_check_fence(fs);

	bnp->count += dir;
	if (level == BOTTOM_LEVEL) {
		if (!alloc)
			return 0;
		secno = cno >> (fs->xf_SectorsPerClusterShift
				+ fs->xf_BytesPerSectorShift + NBBY);
		if ((error = bread(fs->xf_bitmapvp, secno, BN_BLOCK_SIZE, 0,
				   &bp)) != 0)
			return error;
		data = (uint8_t *)bp->b_data;
		if (dir > 0)
			data[remainder / NBBY] |= 1 << (remainder % NBBY);
		else
			data[remainder / NBBY] &= ~(1 << (remainder % NBBY));
		bdwrite(bp);
		exfatfs_check_fence(fs);
		return 0;
	}

	/* If we are full or empty, delete children */
	/* There should be only one */
	if (bnp->count == 0 || BN_FULL(bnp, level)) {
		for (i = 0; i < BN_CHILDREN_COUNT; i++)
			if (bnp->children[i] != NULL)
				pool_put(&exfatfs_bitmap_pool,
					 bnp->children[i]);
		exfatfs_check_fence(fs);
		return 0;
	}

	assert(rcno < BN_CHILDREN_COUNT);
	if (bnp->children[rcno] == NULL) {
		bnp->children[rcno] = pool_get(&exfatfs_bitmap_pool, PR_WAITOK);
		memset(bnp->children[rcno], 0, sizeof(*bnp));
	}
	return exfatfs_bitmap_adj(fs, bnp->children[rcno], cno, --level, dir,
				  alloc);
}

/*
 * Find the next free cluster after "start" and optionally allocate it.
 */
static int
exfatfs_bitmap_scan(struct exfatfs *fs,
		    struct xf_bitmap_node *bnp,
		    uint32_t start, int level,
		    uint32_t *cp,
		    int alloc)
{
	uint32_t i;
	int j;
	struct buf *bp;
	uint8_t *data;
	uint8_t byte;
	int startbit;
	uint32_t r = INVALID;
	uint32_t this_mask = ((1 << (level * BN_CHILDREN_SHIFT)) - 1);
	uint32_t next_mask = ((1 << ((level - 1) * BN_CHILDREN_SHIFT)) - 1);
	uint32_t remainder = start & this_mask;
	uint32_t rcno = ((start & this_mask) & ~next_mask)
		>> ((level - 1) * BN_CHILDREN_SHIFT);
	daddr_t secno;
	int error;
	
	exfatfs_check_fence(fs);
	if (level == BOTTOM_LEVEL) {
		secno = start >> (fs->xf_SectorsPerClusterShift
				  + fs->xf_BytesPerSectorShift + NBBY);
		if ((error = bread(fs->xf_bitmapvp, secno, BN_BLOCK_SIZE, 0,
				   &bp)) != 0)
			return error;
		data = (uint8_t *)bp->b_data;
		r = INVALID;
		for (i = remainder / NBBY; i < BN_BLOCK_SIZE; i++) {
			byte = data[i];
			startbit = 1;
			if (i == remainder / NBBY)
				startbit = remainder % NBBY;
			for (j = startbit; j < NBBY; ++j) {
				if ((byte & (1 << j)) == 0) {
					r = i * NBBY + j;
					break;
				}
			}
			if (r != INVALID)
				break;
		}
		if (alloc && r != INVALID) {
			DPRINTF(("TRIE: allocate cluster %u at lbn %u byte %d"
				 " bit %d\n", (unsigned)r,
				 (unsigned)bp->b_lblkno, (int)i, (int)j));
			data[i] |= (1 << j);
			bdwrite(bp);
		} else
			brelse(bp, 0);
		*cp = r;
		exfatfs_check_fence(fs);
		return 0;
	}

	if (BN_FULL(bnp, level)) {
		*cp = INVALID;
		return ENOSPC;
	}

	for (i = rcno; i < BN_CHILDREN_COUNT; ++i) {
		if (bnp->children[i] == NULL) /* Empty (or full) section */
			return (i << (BN_CHILDREN_SHIFT * level));

		error = exfatfs_bitmap_scan(fs, bnp->children[i], start,
					    --level, &r, alloc);
		if (error == 0 && r != INVALID) {
			*cp = (i << (BN_CHILDREN_SHIFT * level)) | r;
			exfatfs_check_fence(fs);
			return 0;
		}
	}

	/* Not reached? */
	*cp = INVALID;
	exfatfs_check_fence(fs);
	return ENOSPC;
}

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
			 (unsigned long long)
				(1 << (i * BN_CHILDREN_SHIFT)) - 1));
		if (fs->xf_ClusterCount < (1 << (i * BN_CHILDREN_SHIFT)) - 1) {
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
	for (i = 0; i < VTOXI(fs->xf_bitmapvp)->xi_validDataLength;
	     i += SECSIZE(fs)) {
		if ((error = bread(fs->xf_bitmapvp,
				   i >> fs->xf_BytesPerSectorShift,
				   SECSIZE(fs), 0, &bp)) != 0)
			return error;
		DPRINTF(("TRIE: read lblkno %lu blkno 0x%lx\n",
		       (unsigned long)bp->b_lblkno,
			 (unsigned long)bp->b_blkno));
		for (j = 0; j < SECSIZE(fs); j++) {
			cno = i * (SECSIZE(fs) * NBBY) + j;
			if (cno > fs->xf_ClusterCount)
				break;
			byte = ((char *)bp->b_data)[j];
			if (cno + NBBY > fs->xf_ClusterCount)
				byte &= ((1 << (fs->xf_ClusterCount
						& (NBBY - 1))) - 1);
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

int
exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp)
{
	return exfatfs_bitmap_scan(fs, fs->xf_bitmap_trie, start,
				   fs->xf_bitmap_toplevel, cp, 1);
}

int
exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno)
{
	return exfatfs_bitmap_adj(fs, fs->xf_bitmap_trie, cno,
				  fs->xf_bitmap_toplevel, -1, 1);
}

int
exfatfs_bitmap_init(struct exfatfs *fs, int doread)
{
	return exfatfs_init_trie(fs, doread);
}

void
exfatfs_bitmap_destroy(struct exfatfs *fs)
{
	exfatfs_free_bitmap_recurse(fs->xf_bitmap_trie);
}
#else /* ! EXFATFS_USE_TRIE */

/*
#define setbit(a,i)     ((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define clrbit(a,i)     ((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define isset(a,i)      ((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define isclr(a,i)      (((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)
*/
/*
 * A simplistic version in case the TRIE version doesn't work.
 */
int
exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp)
{
	daddr_t lbn, blkno;
	uint32_t r, ostart, end;
	struct buf *bp;
	int off, error, c;
	uint8_t *data;

	if (start < 2)
		start = 2;
	
	DPRINTF(("basic allocate cluster start=%u\n", (unsigned)start));

	assert(fs->xf_bitmapvp != NULL);
	
	ostart = start;
	end = fs->xf_ClusterCount + 2;
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
			if ((error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0,
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
				assert(r >= 2 && r < fs->xf_ClusterCount - 2);
				DPRINTF(("basic allocate cluster %u/%u at lbn"
					 " %u bit %d\n", (unsigned)r,
					 (unsigned)end, (unsigned)lbn,
					 (int)off));
				setbit(data, off);
				bdwrite(bp);
#ifdef _KERNEL
				mutex_enter(&fs->xf_lock);
#endif /* _KERNEL */
				--fs->xf_FreeClusterCount;
#ifdef _KERNEL
				mutex_exit(&fs->xf_lock);
#endif /* _KERNEL */
				*cp = r;
				return 0;
			} else
				brelse(bp, 0);
			++lbn;
			off = 0;
#ifdef _KERNEL
			mutex_enter(&fs->xf_lock);
#endif /* _KERNEL */
			assert(++c <= 2 * fs->xf_ClusterCount);
#ifdef _KERNEL
			mutex_exit(&fs->xf_lock);
#endif /* _KERNEL */
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

int
exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno)
{
	daddr_t lbn, blkno;
	struct buf *bp;
	int off, error;
	uint8_t *data;
	
	assert(fs->xf_bitmapvp != NULL);
	
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
exfatfs_bitmap_init(struct exfatfs *fs, int doread)
{
	daddr_t lbn, blkno;
	uint32_t end;
	struct buf *bp;
	int off, error;
	uint8_t *data;
	
	assert(fs->xf_bitmapvp != NULL);
	
	/* Scan the bitmap to discover how many clusters are free */
	fs->xf_FreeClusterCount = fs->xf_ClusterCount;

	end = fs->xf_ClusterCount + 2;
	lbn = 0;
	off = 0;
	while (LBNOFF2CLUSTER(fs, lbn, off) < end) {
		exfatfs_bmap_shared(fs->xf_bitmapvp, lbn, NULL, &blkno, NULL);
		DPRINTF((" lbn %u -> bn 0x%x\n",
			 (unsigned)lbn, (unsigned)blkno));
		if ((error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp))
			     != 0) {
			printf("bread(%p, %u, %u) returned %d\n",
			       fs->xf_devvp, (unsigned)blkno,
			       (unsigned)SECSIZE(fs), error);
			return error;
		}
#if 1
		for (data = (uint8_t *)bp->b_data;
		     data - (uint8_t *)bp->b_data < SECSIZE(fs)
			&& LBNOFF2CLUSTER(fs, lbn, off + NBBY) < end;
		     ++data) {
			fs->xf_FreeClusterCount -= byte2bitcount[*data];
			off += NBBY;
		}
#endif
		/* XXX this loop could definitely be improved */
		while (off < (1 << BITMAPSHIFT(fs))
		       && LBNOFF2CLUSTER(fs, lbn, off) < end) {
			data = (uint8_t *)bp->b_data;
			if (isset(data, off))
				--fs->xf_FreeClusterCount;
			++off;
		}
		brelse(bp, 0);
		
		++lbn;
		off = 0;
	}

	return 0;
}

void
exfatfs_bitmap_destroy(struct exfatfs *fs)
{
}
#endif /* ! EXFATFS_USE_TRIE */
