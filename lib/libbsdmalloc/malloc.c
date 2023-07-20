/*	$NetBSD: malloc.c,v 1.10 2023/07/08 04:09:26 simonb Exp $	*/

/*
 * Copyright (c) 1983, 1993
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)malloc.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: malloc.c,v 1.10 2023/07/08 04:09:26 simonb Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.  In this
 * implementation, the available sizes are 2^n-4 (or 2^n-10) bytes long.
 * This is designed for use in a virtual memory environment.
 */

#include <sys/types.h>
#if defined(DEBUG) || defined(RCHECK)
#include <sys/uio.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#if defined(RCHECK) || defined(MSTATS)
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reentrant.h"


/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 * If range checking is enabled then a second word holds the size of the
 * requested block, less 1, rounded up to a multiple of sizeof(RMAGIC).
 * The order of elements is critical: ov_magic must overlay the low order
 * bits of ov_next, and ov_magic can not be a valid ov_next bit pattern.
 */
union	overhead {
	union	overhead *ov_next;	/* when free */
	struct {
		u_char	ovu_magic;	/* magic number */
		u_char	ovu_index;	/* bucket # */
#ifdef RCHECK
		u_short	ovu_rmagic;	/* range magic number */
		u_long	ovu_size;	/* actual block size */
#endif
	} ovu;
#define	ov_magic	ovu.ovu_magic
#define	ov_index	ovu.ovu_index
#define	ov_rmagic	ovu.ovu_rmagic
#define	ov_size		ovu.ovu_size
};

#define	MAGIC		0xef		/* magic # on accounting info */
#ifdef RCHECK
#define RMAGIC		0x5555		/* magic # on range info */
#endif

#ifdef RCHECK
#define	RSLOP		sizeof (u_short)
#else
#define	RSLOP		0
#endif

/*
 * nextf[i] is the pointer to the next free block of size 2^(i+3).  The
 * smallest allocatable block is 8 bytes.  The overhead information
 * precedes the data area returned to the user.
 */
#define	NBUCKETS 30
static	union overhead *nextf[NBUCKETS];

static	long pagesz;			/* page size */
static	int pagebucket;			/* page size bucket */

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	u_int nmalloc[NBUCKETS];
#endif

#ifdef _REENT
static	mutex_t malloc_mutex = MUTEX_INITIALIZER;
#endif

static void morecore(int);
static int findbucket(union overhead *, int);
#ifdef MSTATS
void mstats(const char *);
#endif

#if defined(DEBUG) || defined(RCHECK)
#define	ASSERT(p)   if (!(p)) botch(__STRING(p))

static void botch(const char *);

/*
 * NOTE: since this may be called while malloc_mutex is locked, stdio must not
 *       be used in this function.
 */
static void
botch(const char *s)
{
	struct iovec iov[3];

	iov[0].iov_base	= __UNCONST("\nassertion botched: ");
	iov[0].iov_len	= 20;
	iov[1].iov_base	= __UNCONST(s);
	iov[1].iov_len	= strlen(s);
	iov[2].iov_base	= __UNCONST("\n");
	iov[2].iov_len	= 1;

	/*
	 * This place deserves a word of warning: a cancellation point will
	 * occur when executing writev(), and we might be still owning
	 * malloc_mutex.  At this point we need to disable cancellation
	 * until `after' abort() because i) establishing a cancellation handler
	 * might, depending on the implementation, result in another malloc()
	 * to be executed, and ii) it is really not desirable to let execution
	 * continue.  `Fix me.'
	 *
	 * Note that holding mutex_lock during abort() is safe.
	 */

	(void)writev(STDERR_FILENO, iov, 3);
	abort();
}
#else
#define	ASSERT(p)	((void)sizeof((long)(p)))
#endif

void *
malloc(size_t nbytes)
{
  	union overhead *op;
	int bucket;
  	long n;
	unsigned amt;

	mutex_lock(&malloc_mutex);

	/*
	 * First time malloc is called, setup page size and
	 * align break pointer so all data will be page aligned.
	 */
	if (pagesz == 0) {
		pagesz = n = getpagesize();
		ASSERT(pagesz > 0);
		op = (union overhead *)(void *)sbrk(0);
  		n = n - sizeof (*op) - ((long)op & (n - 1));
		if (n < 0)
			n += pagesz;
		if (n) {
			if (sbrk((int)n) == (void *)-1) {
				mutex_unlock(&malloc_mutex);
				return (NULL);
			}
		}
		bucket = 0;
		amt = 8;
		while (pagesz > amt) {
			amt <<= 1;
			bucket++;
		}
		pagebucket = bucket;
	}
	/*
	 * Convert amount of memory requested into closest block size
	 * stored in hash buckets which satisfies request.
	 * Account for space used per block for accounting.
	 */
	if (nbytes <= (n = pagesz - sizeof (*op) - RSLOP)) {
#ifndef RCHECK
		amt = 8;	/* size of first bucket */
		bucket = 0;
#else
		amt = 16;	/* size of first bucket */
		bucket = 1;
#endif
		n = -((long)sizeof (*op) + RSLOP);
	} else {
		amt = (unsigned)pagesz;
		bucket = pagebucket;
	}
	while (nbytes > amt + n) {
		amt <<= 1;
		if (amt == 0)
			return (NULL);
		bucket++;
	}
	/*
	 * If nothing in hash bucket right now,
	 * request more memory from the system.
	 */
  	if ((op = nextf[bucket]) == NULL) {
  		morecore(bucket);
  		if ((op = nextf[bucket]) == NULL) {
			mutex_unlock(&malloc_mutex);
  			return (NULL);
		}
	}
	/* remove from linked list */
  	nextf[bucket] = op->ov_next;
	op->ov_magic = MAGIC;
	op->ov_index = bucket;
#ifdef MSTATS
  	nmalloc[bucket]++;
#endif
	mutex_unlock(&malloc_mutex);
#ifdef RCHECK
	/*
	 * Record allocated size of block and
	 * bound space with magic numbers.
	 */
	op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
	op->ov_rmagic = RMAGIC;
  	*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
  	return ((void *)(op + 1));
}

/*
 * Allocate more memory to the indicated bucket.
 */
static void
morecore(int bucket)
{
  	union overhead *op;
	long sz;		/* size of desired block */
  	long amt;			/* amount to allocate */
  	long nblks;			/* how many blocks we get */

	/*
	 * sbrk_size <= 0 only for big, FLUFFY, requests (about
	 * 2^30 bytes on a VAX, I think) or for a negative arg.
	 */
	sz = 1 << (bucket + 3);
#ifdef DEBUG
	ASSERT(sz > 0);
#else
	if (sz <= 0)
		return;
#endif
	if (sz < pagesz) {
		amt = pagesz;
  		nblks = amt / sz;
	} else {
		amt = sz + pagesz;
		nblks = 1;
	}
	op = (union overhead *)(void *)sbrk((int)amt);
	/* no more room! */
  	if ((long)op == -1)
  		return;
	/*
	 * Add new memory allocated to that on
	 * free list for this hash bucket.
	 */
  	nextf[bucket] = op;
  	while (--nblks > 0) {
		op->ov_next =
		    (union overhead *)(void *)((caddr_t)(void *)op+(size_t)sz);
		op = op->ov_next;
  	}
}

void
free(void *cp)
{
	long size;
	union overhead *op;

  	if (cp == NULL)
  		return;
	op = (union overhead *)(void *)((caddr_t)cp - sizeof (union overhead));
#ifdef DEBUG
  	ASSERT(op->ov_magic == MAGIC);		/* make sure it was in use */
#else
	if (op->ov_magic != MAGIC)
		return;				/* sanity */
#endif
#ifdef RCHECK
  	ASSERT(op->ov_rmagic == RMAGIC);
	ASSERT(*(u_short *)((caddr_t)(op + 1) + op->ov_size) == RMAGIC);
#endif
  	size = op->ov_index;
  	ASSERT(size < NBUCKETS);
	mutex_lock(&malloc_mutex);
	op->ov_next = nextf[(unsigned int)size];/* also clobbers ov_magic */
  	nextf[(unsigned int)size] = op;
#ifdef MSTATS
  	nmalloc[(size_t)size]--;
#endif
	mutex_unlock(&malloc_mutex);
}

/*
 * When a program attempts "storage compaction" as mentioned in the
 * old malloc man page, it realloc's an already freed block.  Usually
 * this is the last block it freed; occasionally it might be farther
 * back.  We have to search all the free lists for the block in order
 * to determine its bucket: 1st we make one pass thru the lists
 * checking only the first block in each; if that fails we search
 * ``__realloc_srchlen'' blocks in each list for a match (the variable
 * is extern so the caller can modify it).  If that fails we just copy
 * however many bytes was given to realloc() and hope it's not huge.
 */
int __realloc_srchlen = 4;	/* 4 should be plenty, -1 =>'s whole list */

void *
realloc(void *cp, size_t nbytes)
{
  	u_long onb;
	long i;
	union overhead *op;
	char *res;
	int was_alloced = 0;

  	if (cp == NULL)
  		return (malloc(nbytes));
	if (nbytes == 0) {
		free (cp);
		return (NULL);
	}
	op = (union overhead *)(void *)((caddr_t)cp - sizeof (union overhead));
	mutex_lock(&malloc_mutex);
	if (op->ov_magic == MAGIC) {
		was_alloced++;
		i = op->ov_index;
	} else {
		/*
		 * Already free, doing "compaction".
		 *
		 * Search for the old block of memory on the
		 * free list.  First, check the most common
		 * case (last element free'd), then (this failing)
		 * the last ``__realloc_srchlen'' items free'd.
		 * If all lookups fail, then assume the size of
		 * the memory block being realloc'd is the
		 * largest possible (so that all "nbytes" of new
		 * memory are copied into).  Note that this could cause
		 * a memory fault if the old area was tiny, and the moon
		 * is gibbous.  However, that is very unlikely.
		 */
		if ((i = findbucket(op, 1)) < 0 &&
		    (i = findbucket(op, __realloc_srchlen)) < 0)
			i = NBUCKETS;
	}
	onb = (u_long)1 << (u_long)(i + 3);
	if (onb < pagesz)
		onb -= sizeof (*op) + RSLOP;
	else
		onb += pagesz - sizeof (*op) - RSLOP;
	/* avoid the copy if same size block */
	if (was_alloced) {
		if (i) {
			i = (long)1 << (long)(i + 2);
			if (i < pagesz)
				i -= sizeof (*op) + RSLOP;
			else
				i += pagesz - sizeof (*op) - RSLOP;
		}
		if (nbytes <= onb && nbytes > i) {
#ifdef RCHECK
			op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
			*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
			mutex_unlock(&malloc_mutex);
			return (cp);

		}
#ifndef _REENT
		else
			free(cp);
#endif
	}
	mutex_unlock(&malloc_mutex);
	if ((res = malloc(nbytes)) == NULL) {
#ifdef _REENT
		free(cp);
#endif
		return (NULL);
	}
#ifndef _REENT
	if (cp != res)		/* common optimization if "compacting" */
		(void)memmove(res, cp, (size_t)((nbytes < onb) ? nbytes : onb));
#else
	(void)memmove(res, cp, (size_t)((nbytes < onb) ? nbytes : onb));
	free(cp);
#endif
  	return (res);
}

/*
 * Search ``srchlen'' elements of each free list for a block whose
 * header starts at ``freep''.  If srchlen is -1 search the whole list.
 * Return bucket number, or -1 if not found.
 */
static int
findbucket(union overhead *freep, int srchlen)
{
	union overhead *p;
	int i, j;

	for (i = 0; i < NBUCKETS; i++) {
		j = 0;
		for (p = nextf[i]; p && j != srchlen; p = p->ov_next) {
			if (p == freep)
				return (i);
			j++;
		}
	}
	return (-1);
}

#ifdef MSTATS
/*
 * mstats - print out statistics about malloc
 *
 * Prints two lines of numbers, one showing the length of the free list
 * for each size category, the second showing the number of mallocs -
 * frees for each size category.
 */
void
mstats(const char *s)
{
  	int i, j;
  	union overhead *p;
  	int totfree = 0,
  	totused = 0;

  	fprintf(stderr, "Memory allocation statistics %s\nfree:\t", s);
  	for (i = 0; i < NBUCKETS; i++) {
  		for (j = 0, p = nextf[i]; p; p = p->ov_next, j++)
  			;
  		fprintf(stderr, " %d", j);
  		totfree += j * (1 << (i + 3));
  	}
  	fprintf(stderr, "\nused:\t");
  	for (i = 0; i < NBUCKETS; i++) {
  		fprintf(stderr, " %d", nmalloc[i]);
  		totused += nmalloc[i] * (1 << (i + 3));
  	}
  	fprintf(stderr, "\n\tTotal in use: %d, total free: %d\n",
	    totused, totfree);
}
#endif

/*
 * Additional front ends:
 * - aligned_alloc (C11)
 * - calloc(n,m) = malloc(n*m) without overflow
 * - posix_memalign (POSIX)
 *
 * These must all be in the same compilation unit as malloc, realloc,
 * and free (or -lbsdmalloc must be surrounded by -Wl,--whole-archive
 * -lbsdmalloc -Wl,--no-whole-archive) in order to override the libc
 * built-in malloc implementation.
 *
 * Allocations of size n, up to and including the page size, are
 * already aligned by malloc on multiples of n.  Larger alignment is
 * not supported.
 */

static long __constfunc
cachedpagesize(void)
{
	long n;

	/* XXX atomic_load_relaxed, but that's not defined in userland atm */
	if (__predict_false((n = pagesz) == 0)) {
		mutex_lock(&malloc_mutex);
		if ((n = pagesz) == 0)
			n = pagesz = getpagesize();
		mutex_unlock(&malloc_mutex);
	}

	return n;
}

void *
aligned_alloc(size_t alignment, size_t size)
{
	char *p;

	if (alignment == 0 ||
	    (alignment & (alignment - 1)) != 0 ||
	    alignment > cachedpagesize()) {
		errno = EINVAL;
		return NULL;
	}
	p = malloc(size < alignment ? alignment : size);
	if (__predict_false(p == NULL))
		ASSERT((uintptr_t)p % alignment == 0);
	return p;
}

void *
calloc(size_t nmemb, size_t size)
{
	void *p;
	size_t n;

	if (__builtin_mul_overflow(nmemb, size, &n)) {
		errno = ENOMEM;
		return NULL;
	}
	p = malloc(n);
	if (__predict_false(p == NULL))
		return NULL;
	memset(p, 0, n);
	return p;
}

int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
	char *p;

	if (alignment < sizeof(void *) ||
	    (alignment & (alignment - 1)) != 0 ||
	    alignment > cachedpagesize())
		return EINVAL;
	p = malloc(size < alignment ? alignment : size);
	if (__predict_false(p == NULL))
		return ENOMEM;
	ASSERT((uintptr_t)p % alignment == 0);
	*memptr = p;
	return 0;
}

/*
 * libc hooks required by fork
 */

#include "../libc/include/extern.h"

void
_malloc_prefork(void)
{

	mutex_lock(&malloc_mutex);
}

void
_malloc_postfork(void)
{

	mutex_unlock(&malloc_mutex);
}

void
_malloc_postfork_child(void)
{

	mutex_unlock(&malloc_mutex);
}
