/*	$NetBSD: malloc.h,v 1.89 2003/08/07 16:34:07 agc Exp $	*/

/*
 * Copyright (c) 1987, 1993
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
 *
 *	@(#)malloc.h	8.5 (Berkeley) 5/3/95
 */

#ifndef _SYS_MALLOC_H_
#define	_SYS_MALLOC_H_

#if defined(_KERNEL_OPT)
#include "opt_kmemstats.h"
#include "opt_malloclog.h"
#include "opt_malloc_debug.h"
#include "opt_lockdebug.h"
#endif


/*
 * flags to malloc
 */
#define	M_WAITOK	0x0000	/* can wait for resources */
#define	M_NOWAIT	0x0001	/* do not wait for resources */
#define	M_ZERO		0x0002	/* zero the allocation */
#define	M_CANFAIL	0x0004	/* can fail if requested memory can't ever
				 * be allocated */
#ifdef _KERNEL

#include <sys/lock.h>
#include <sys/mallocvar.h>
/*
 * The following are standard, built-in malloc types that are
 * not specific to any one subsystem.
 */
MALLOC_DECLARE(M_DEVBUF);
MALLOC_DECLARE(M_DMAMAP);
MALLOC_DECLARE(M_FREE);
MALLOC_DECLARE(M_PCB);
MALLOC_DECLARE(M_SOFTINTR);
MALLOC_DECLARE(M_TEMP);

/* XXX These should all be declared elsewhere. */
MALLOC_DECLARE(M_RTABLE);
MALLOC_DECLARE(M_FTABLE);
MALLOC_DECLARE(M_UFSMNT);
MALLOC_DECLARE(M_NETADDR);
MALLOC_DECLARE(M_IPMOPTS);
MALLOC_DECLARE(M_IPMADDR);
MALLOC_DECLARE(M_MRTABLE);
MALLOC_DECLARE(M_1394DATA);
#endif /* _KERNEL */

/*
 * Array of descriptors that describe the contents of each page
 */
struct kmemusage {
	short ku_indx;		/* bucket index */
	union {
		u_short freecnt;/* for small allocations, free pieces in page */
		u_short pagecnt;/* for large allocations, pages alloced */
	} ku_un;
};
#define	ku_freecnt ku_un.freecnt
#define	ku_pagecnt ku_un.pagecnt

/*
 * Set of buckets for each size of memory block that is retained
 */
struct kmembuckets {
	caddr_t kb_next;	/* list of free blocks */
	caddr_t kb_last;	/* last free block */
	long	kb_calls;	/* total calls to allocate this size */
	long	kb_total;	/* total number of blocks allocated */
	long	kb_totalfree;	/* # of free elements in this bucket */
	long	kb_elmpercl;	/* # of elements in this sized allocation */
	long	kb_highwat;	/* high water mark */
	long	kb_couldfree;	/* over high water mark and could free */
};

#ifdef _KERNEL
#define	MINALLOCSIZE	(1 << MINBUCKET)
#define	BUCKETINDX(size) \
	((size) <= (MINALLOCSIZE * 128) \
		? (size) <= (MINALLOCSIZE * 8) \
			? (size) <= (MINALLOCSIZE * 2) \
				? (size) <= (MINALLOCSIZE * 1) \
					? (MINBUCKET + 0) \
					: (MINBUCKET + 1) \
				: (size) <= (MINALLOCSIZE * 4) \
					? (MINBUCKET + 2) \
					: (MINBUCKET + 3) \
			: (size) <= (MINALLOCSIZE* 32) \
				? (size) <= (MINALLOCSIZE * 16) \
					? (MINBUCKET + 4) \
					: (MINBUCKET + 5) \
				: (size) <= (MINALLOCSIZE * 64) \
					? (MINBUCKET + 6) \
					: (MINBUCKET + 7) \
		: (size) <= (MINALLOCSIZE * 2048) \
			? (size) <= (MINALLOCSIZE * 512) \
				? (size) <= (MINALLOCSIZE * 256) \
					? (MINBUCKET + 8) \
					: (MINBUCKET + 9) \
				: (size) <= (MINALLOCSIZE * 1024) \
					? (MINBUCKET + 10) \
					: (MINBUCKET + 11) \
			: (size) <= (MINALLOCSIZE * 8192) \
				? (size) <= (MINALLOCSIZE * 4096) \
					? (MINBUCKET + 12) \
					: (MINBUCKET + 13) \
				: (size) <= (MINALLOCSIZE * 16384) \
					? (MINBUCKET + 14) \
					: (MINBUCKET + 15))

/*
 * Turn virtual addresses into kmem map indicies
 */
#define	kmemxtob(alloc)	(kmembase + (alloc) * NBPG)
#define	btokmemx(addr)	(((caddr_t)(addr) - kmembase) / NBPG)
#define	btokup(addr)	(&kmemusage[((caddr_t)(addr) - kmembase) >> PGSHIFT])

extern struct simplelock malloc_slock;

/*
 * Macro versions for the usual cases of malloc/free
 */
#if defined(KMEMSTATS) || defined(DIAGNOSTIC) || defined(_LKM) || \
    defined(MALLOCLOG) || defined(LOCKDEBUG) || defined(MALLOC_NOINLINE)
#define	MALLOC(space, cast, size, type, flags) \
	(space) = (cast)malloc((u_long)(size), (type), (flags))
#define	FREE(addr, type) free((caddr_t)(addr), (type))

#else /* do not collect statistics */
#define	MALLOC(space, cast, size, type, flags)				\
do {									\
	register struct kmembuckets *__kbp = &bucket[BUCKETINDX((size))]; \
	long __s = splvm();						\
	simple_lock(&malloc_slock);					\
	if (__kbp->kb_next == NULL) {					\
		simple_unlock(&malloc_slock);				\
		(space) = (cast)malloc((u_long)(size), (type), (flags)); \
		splx(__s);						\
	} else {							\
		(space) = (cast)__kbp->kb_next;				\
		__kbp->kb_next = *(caddr_t *)(space);			\
		simple_unlock(&malloc_slock);				\
		splx(__s);						\
		if ((flags) & M_ZERO)					\
			memset((space), 0, (size));			\
	}								\
} while (/* CONSTCOND */ 0)

#define	FREE(addr, type)						\
do {									\
	register struct kmembuckets *__kbp;				\
	register struct kmemusage *__kup = btokup((addr));		\
	long __s = splvm();						\
	if (1 << __kup->ku_indx > MAXALLOCSAVE) {			\
		free((caddr_t)(addr), (type));				\
	} else {							\
		simple_lock(&malloc_slock);				\
		__kbp = &bucket[__kup->ku_indx];			\
		if (__kbp->kb_next == NULL)				\
			__kbp->kb_next = (caddr_t)(addr);		\
		else							\
			*(caddr_t *)(__kbp->kb_last) = (caddr_t)(addr);	\
		*(caddr_t *)(addr) = NULL;				\
		__kbp->kb_last = (caddr_t)(addr);			\
		simple_unlock(&malloc_slock);				\
	}								\
	splx(__s);							\
} while(/* CONSTCOND */ 0)
#endif /* do not collect statistics */

extern struct kmemusage		*kmemusage;
extern char			*kmembase;
extern struct kmembuckets	bucket[];

#ifdef MALLOCLOG
void	*_malloc(unsigned long, struct malloc_type *, int, const char *, long);
void	_free(void *, struct malloc_type *, const char *, long);
#define	malloc(size, type, flags) \
	    _malloc((size), (type), (flags), __FILE__, __LINE__)
#define	free(addr, type) \
	    _free((addr), (type), __FILE__, __LINE__)
#else
void	*malloc(unsigned long, struct malloc_type *, int);
void	free(void *, struct malloc_type *);
#endif /* MALLOCLOG */

#ifdef MALLOC_DEBUG
int	debug_malloc(unsigned long, struct malloc_type *, int, void **);
int	debug_free(void *, struct malloc_type *);
void	debug_malloc_init(void);

void	debug_malloc_print(void);
void	debug_malloc_printit(void (*)(const char *, ...), vaddr_t);
#endif /* MALLOC_DEBUG */

void	*realloc(void *, unsigned long, struct malloc_type *, int);
unsigned long
	malloc_roundup(unsigned long);
#endif /* _KERNEL */
#endif /* !_SYS_MALLOC_H_ */
