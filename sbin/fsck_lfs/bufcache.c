/* $NetBSD: bufcache.c,v 1.2 2003/03/31 19:56:59 perseant Exp $ */
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

#define HASH_MAX 101

struct bufhash_struct bufhash[HASH_MAX];

int maxbufs = BUF_CACHE_SIZE;
int nbufs = 0;
int cachehits = 0;
int cachemisses = 0;
int hashmax = 0;
off_t locked_queue_bytes = 0;

/* Simple buffer hash function */
static int
vl_hash(struct uvnode * vp, daddr_t lbn)
{
	return (int)((unsigned long) vp + lbn) % HASH_MAX;
}

/* Initialize buffer cache */
void
bufinit(void)
{
	int i;

	for (i = 0; i < BQUEUES; i++) {
		TAILQ_INIT(&bufqueues[i]);
	}
	for (i = 0; i < HASH_MAX; i++)
		LIST_INIT(&bufhash[HASH_MAX]);
}

/* Print statistics of buffer cache usage */
void
bufstats(void)
{
	printf("buffer cache: %d hits %d misses (%2.2f%%); hash depth %d\n",
	    cachehits, cachemisses,
	    (cachehits * 100.0) / (cachehits + cachemisses),
	    hashmax);
}

/*
 * Remove a buffer from the cache.
 * Caller must remove the buffer from its free list.
 */
void
buf_destroy(struct ubuf * bp)
{
	bp->b_flags |= B_NEEDCOMMIT;
	LIST_REMOVE(bp, b_vnbufs);
	LIST_REMOVE(bp, b_hash);
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
	 *
	 * NB: This makes an assumption about how tailq's are implemented.
	 */
	if (bp->b_flags & B_LOCKED) {
		locked_queue_bytes -= bp->b_bcount;
	}
	if (TAILQ_NEXT(bp, b_freelist) == NULL) {
		for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
			if (dp->tqh_last == &bp->b_freelist.tqe_next)
				break;
		if (dp == &bufqueues[BQUEUES])
			errx(1, "bremfree: lost tail");
	}
	++bp->b_vp->v_usecount;
	TAILQ_REMOVE(dp, bp, b_freelist);
}

/* Return a buffer if it is in the cache, otherwise return NULL. */
struct ubuf *
incore(struct uvnode * vp, int lbn)
{
	struct ubuf *bp;
	int hash, depth;

	hash = vl_hash(vp, lbn);
	/* XXX use a real hash instead. */
	depth = 0;
	LIST_FOREACH(bp, &bufhash[hash], b_hash) {
		if (++depth > hashmax)
			hashmax = depth;
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

	/*
	 * First check the buffer cache lists.
	 */
	if ((bp = incore(vp, lbn)) != NULL) {
		assert(!(bp->b_flags & B_NEEDCOMMIT));
		assert(!(bp->b_flags & B_BUSY));
		assert(bp->b_bcount == size);
		bp->b_flags |= B_BUSY;
		bremfree(bp);
		return bp;
	}
	/*
	 * Not on the list.
	 * Get a new block of the appropriate size and use that.
	 * If not enough space, free blocks from the AGE and LRU lists
	 * to make room.
	 */
	while (nbufs >= maxbufs) {
		bp = TAILQ_FIRST(&bufqueues[BQ_AGE]);
		if (bp)
			TAILQ_REMOVE(&bufqueues[BQ_AGE], bp, b_freelist);
		if (bp == NULL) {
			bp = TAILQ_FIRST(&bufqueues[BQ_LRU]);
			if (bp)
				TAILQ_REMOVE(&bufqueues[BQ_AGE], bp,
				    b_freelist);
		}
		if (bp) {
			if (bp->b_flags & B_DELWRI)
				VOP_STRATEGY(bp);
			buf_destroy(bp);
		}
#ifdef DEBUG
		else {
			warnx("no free buffers, allocating more than %d",
			    maxbufs);
		}
#endif
	}
	++nbufs;
	bp = (struct ubuf *) malloc(sizeof(*bp));
	memset(bp, 0, sizeof(*bp));
	bp->b_data = malloc(size);
	memset(bp->b_data, 0, size);

	bp->b_vp = vp;
	bp->b_blkno = bp->b_lblkno = lbn;
	bp->b_bcount = size;
	LIST_INSERT_HEAD(&bufhash[vl_hash(vp, lbn)], bp, b_hash);
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
	brelse(bp);
}

/* Put a buffer back on its free list, clear B_BUSY. */
void
brelse(struct ubuf * bp)
{
	int age;

	assert(!(bp->b_flags & B_NEEDCOMMIT));
	assert(bp->b_flags & B_BUSY);

	age = bp->b_flags & B_AGE;
	bp->b_flags &= ~(B_BUSY | B_AGE);
	if (bp->b_flags & B_INVAL) {
		buf_destroy(bp);
		return;
	}
	if (bp->b_flags & B_LOCKED) {
		locked_queue_bytes += bp->b_bcount;
		TAILQ_INSERT_TAIL(&bufqueues[BQ_LOCKED], bp, b_freelist);
	} else if (age) {
		TAILQ_INSERT_TAIL(&bufqueues[BQ_AGE], bp, b_freelist);
	} else {
		TAILQ_INSERT_TAIL(&bufqueues[BQ_LRU], bp, b_freelist);
	}
	--bp->b_vp->v_usecount;
}

/* Read the given block from disk, return it B_BUSY. */
int
bread(struct uvnode * vp, daddr_t lbn, int size, struct ucred * unused,
    struct ubuf ** bpp)
{
	struct ubuf *bp;
	daddr_t daddr;
	int error, count;

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
	error = VOP_BMAP(vp, lbn, &daddr);
	bp->b_blkno = daddr;
	if (daddr >= 0) {
		count = pread(vp->v_fd, bp->b_data, bp->b_bcount,
				dbtob((off_t) daddr));
		if (count == bp->b_bcount) {
			bp->b_flags |= B_DONE;
			return 0;
		}
		return -1;
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
