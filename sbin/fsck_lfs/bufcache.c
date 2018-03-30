/* $NetBSD: bufcache.c,v 1.20 2018/03/30 12:56:46 christos Exp $ */
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
#include <sys/queue.h>
#include <sys/mount.h>

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"

/*
 * Definitions for the buffer free lists.
 */
#define	BQUEUES		3	/* number of free buffer queues */

#define	BQ_LOCKED	0	/* super-blocks &c */
#define	BQ_LRU		1	/* lru, useful buffers */
#define	BQ_AGE		2	/* rubbish */

TAILQ_HEAD(bqueues, ubuf) bufqueues[BQUEUES];

struct bufhash_struct *bufhash;

#define HASH_MAX 1024
int hashmax  = HASH_MAX;
int hashmask = (HASH_MAX - 1);

int maxbufs = BUF_CACHE_SIZE;
int nbufs = 0;
int cachehits = 0;
int cachemisses = 0;
int max_depth = 0;
off_t locked_queue_bytes = 0;
int locked_queue_count = 0;

/* Simple buffer hash function */
static int
vl_hash(struct uvnode * vp, daddr_t lbn)
{
	return (int)((unsigned long) vp + lbn) & hashmask;
}

/* Initialize buffer cache */
void
bufinit(int max)
{
	int i;

	if (max) {
		for (hashmax = 1; hashmax < max; hashmax <<= 1)
			;
		hashmask = hashmax - 1;
	}

	for (i = 0; i < BQUEUES; i++) {
		TAILQ_INIT(&bufqueues[i]);
	}
	bufhash = emalloc(hashmax * sizeof(*bufhash));
	for (i = 0; i < hashmax; i++)
		LIST_INIT(&bufhash[i]);
}

/* Widen the hash table. */
void bufrehash(int max)
{
	int i, newhashmax;
	struct ubuf *bp, *nbp;
	struct bufhash_struct *np;

	if (max < 0 || max <= hashmax)
		return;

	/* Round up to a power of two */
	for (newhashmax = 1; newhashmax < max; newhashmax <<= 1)
		;

	/* update the mask right away so vl_hash() uses it */
	hashmask = newhashmax - 1;

	/* Allocate new empty hash table, if we can */
	np = emalloc(newhashmax * sizeof(*bufhash));
	for (i = 0; i < newhashmax; i++)
		LIST_INIT(&np[i]);

	/* Now reassign all existing buffers to their new hash chains. */
	for (i = 0; i < hashmax; i++) {
		bp = LIST_FIRST(&bufhash[i]);
		while(bp) {
			nbp = LIST_NEXT(bp, b_hash);
			LIST_REMOVE(bp, b_hash);
			bp->b_hashval = vl_hash(bp->b_vp, bp->b_lblkno);
			LIST_INSERT_HEAD(&np[bp->b_hashval], bp, b_hash);
			bp = nbp;
		}
	}

	/* Switch over and clean up */
	free(bufhash);
	bufhash = np;
	hashmax = newhashmax;
}

/* Print statistics of buffer cache usage */
void
bufstats(void)
{
	printf("buffer cache: %d hits %d misses (%2.2f%%); hash width %d, depth %d\n",
	    cachehits, cachemisses,
	    (cachehits * 100.0) / (cachehits + cachemisses),
	    hashmax, max_depth);
}

/*
 * Remove a buffer from the cache.
 * Caller must remove the buffer from its free list.
 */
void
buf_destroy(struct ubuf * bp)
{
	LIST_REMOVE(bp, b_vnbufs);
	LIST_REMOVE(bp, b_hash);
	if (!(bp->b_flags & B_DONTFREE))
		free(bp->b_data);
	free(bp);
	--nbufs;
}

/* Remove a buffer from its free list. */
void
bremfree(struct ubuf * bp)
{
	struct bqueues *dp = NULL;

	/*
	 * We only calculate the head of the freelist when removing
	 * the last element of the list as that is the only time that
	 * it is needed (e.g. to reset the tail pointer).
	 */
	if (bp->b_flags & B_LOCKED) {
		locked_queue_bytes -= bp->b_bcount;
		--locked_queue_count;
	}
	if (TAILQ_NEXT(bp, b_freelist) == NULL) {
		for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
			if (TAILQ_LAST(dp, bqueues) == bp)
				break;
		if (dp == &bufqueues[BQUEUES])
			errx(1, "bremfree: lost tail");
	}
	++bp->b_vp->v_usecount;
	TAILQ_REMOVE(dp, bp, b_freelist);
}

/* Return a buffer if it is in the cache, otherwise return NULL. */
struct ubuf *
incore(struct uvnode * vp, daddr_t lbn)
{
	struct ubuf *bp;
	int hash, depth;

	hash = vl_hash(vp, lbn);
	/* XXX use a real hash instead. */
	depth = 0;
	LIST_FOREACH(bp, &bufhash[hash], b_hash) {
		if (++depth > max_depth)
			max_depth = depth;
		assert(depth <= nbufs);
		if (bp->b_vp == vp && bp->b_lblkno == lbn) {
			return bp;
		}
	}
	return NULL;
}

/*
 * Return a buffer of the given size, lbn and uvnode.
 * If none is in core, make a new one.
 */
struct ubuf *
getblk(struct uvnode * vp, daddr_t lbn, int size)
{
	struct ubuf *bp;
#ifdef DEBUG
	static int warned;
#endif

	/*
	 * First check the buffer cache lists.
	 * We might sometimes need to resize a buffer.  If we are growing
	 * the buffer, its contents are invalid; but shrinking is okay.
	 */
	if ((bp = incore(vp, lbn)) != NULL) {
		assert(!(bp->b_flags & B_BUSY));
		bp->b_flags |= B_BUSY;
		bremfree(bp);
		if (bp->b_bcount == size)
			return bp;
		else if (bp->b_bcount > size) {
			assert(!(bp->b_flags & B_DELWRI));
			bp->b_bcount = size;
			bp->b_data = erealloc(bp->b_data, size);
			return bp;
		}

		buf_destroy(bp);
		bp = NULL;
	}

	/*
	 * Not on the list.
	 * Get a new block of the appropriate size and use that.
	 * If not enough space, free blocks from the AGE and LRU lists
	 * to make room.
	 */
	while (nbufs >= maxbufs + locked_queue_count) {
		bp = TAILQ_FIRST(&bufqueues[BQ_AGE]);
		if (bp)
			TAILQ_REMOVE(&bufqueues[BQ_AGE], bp, b_freelist);
		if (bp == NULL) {
			bp = TAILQ_FIRST(&bufqueues[BQ_LRU]);
			if (bp)
				TAILQ_REMOVE(&bufqueues[BQ_LRU], bp,
				    b_freelist);
		}
		if (bp) {
			if (bp->b_flags & B_DELWRI)
				VOP_STRATEGY(bp);
			buf_destroy(bp);
			break;
		}
#ifdef DEBUG
		else if (!warned) {
			warnx("allocating more than %d buffers", maxbufs);
			++warned;
		}
#endif
		break;
	}
	++nbufs;
	bp = ecalloc(1, sizeof(*bp));
	bp->b_data = ecalloc(1, size);
	bp->b_vp = vp;
	bp->b_blkno = bp->b_lblkno = lbn;
	bp->b_bcount = size;
	bp->b_hashval = vl_hash(vp, lbn);
	LIST_INSERT_HEAD(&bufhash[bp->b_hashval], bp, b_hash);
	LIST_INSERT_HEAD(&vp->v_cleanblkhd, bp, b_vnbufs);
	bp->b_flags = B_BUSY;

	return bp;
}

/* Write a buffer to disk according to its strategy routine. */
void
bwrite(struct ubuf * bp)
{
	bp->b_flags &= ~(B_READ | B_DONE | B_DELWRI | B_LOCKED);
	VOP_STRATEGY(bp);
	bp->b_flags |= B_DONE;
	reassignbuf(bp, bp->b_vp);
	brelse(bp, 0);
}

/* Put a buffer back on its free list, clear B_BUSY. */
void
brelse(struct ubuf * bp, int set)
{
	int age;

	assert(bp->b_flags & B_BUSY);

	bp->b_flags |= set;

	age = bp->b_flags & B_AGE;
	bp->b_flags &= ~(B_BUSY | B_AGE);
	if (bp->b_flags & B_INVAL) {
		buf_destroy(bp);
		return;
	}
	if (bp->b_flags & B_LOCKED) {
		locked_queue_bytes += bp->b_bcount;
		++locked_queue_count;
		TAILQ_INSERT_TAIL(&bufqueues[BQ_LOCKED], bp, b_freelist);
	} else if (age) {
		TAILQ_INSERT_TAIL(&bufqueues[BQ_AGE], bp, b_freelist);
	} else {
		TAILQ_INSERT_TAIL(&bufqueues[BQ_LRU], bp, b_freelist);
	}
	--bp->b_vp->v_usecount;

	/* Move to the front of the hash chain */
	if (LIST_FIRST(&bufhash[bp->b_hashval]) != bp) {
		LIST_REMOVE(bp, b_hash);
		LIST_INSERT_HEAD(&bufhash[bp->b_hashval], bp, b_hash);
	}
}

/* Read the given block from disk, return it B_BUSY. */
int
bread(struct uvnode * vp, daddr_t lbn, int size, int flags, struct ubuf ** bpp)
{
	struct ubuf *bp;
	daddr_t daddr;

	bp = getblk(vp, lbn, size);
	*bpp = bp;
	if (bp->b_flags & (B_DELWRI | B_DONE)) {
		++cachehits;
		return 0;
	}
	++cachemisses;

	/*
	 * Not found.  Need to find that block's location on disk,
	 * and load it in.
	 */
	daddr = -1;
	(void)VOP_BMAP(vp, lbn, &daddr);
	bp->b_blkno = daddr;
	if (daddr >= 0) {
		bp->b_flags |= B_READ;
		return VOP_STRATEGY(bp);
	}
	memset(bp->b_data, 0, bp->b_bcount);
	return 0;
}

/* Move a buffer between dirty and clean block lists. */
void
reassignbuf(struct ubuf * bp, struct uvnode * vp)
{
	LIST_REMOVE(bp, b_vnbufs);
	if (bp->b_flags & B_DELWRI) {
		LIST_INSERT_HEAD(&vp->v_dirtyblkhd, bp, b_vnbufs);
	} else {
		LIST_INSERT_HEAD(&vp->v_cleanblkhd, bp, b_vnbufs);
	}
}

#ifdef DEBUG
void
dump_free_lists(void)
{
	struct ubuf *bp;
	int i;

	for (i = 0; i <= BQ_LOCKED; i++) {
		printf("==> free list %d:\n", i);
		TAILQ_FOREACH(bp, &bufqueues[i], b_freelist) {
			printf("vp %p lbn %" PRId64 " flags %lx\n",
				bp->b_vp, bp->b_lblkno, bp->b_flags);
		}
	}
}
#endif
