/*	$NetBSD: malloc.h,v 1.106 2009/11/06 13:32:41 pooka Exp $	*/

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
#endif


#ifdef _KERNEL

/*
 * flags to malloc
 */
#define	M_WAITOK	0x0000	/* can wait for resources */
#define	M_NOWAIT	0x0001	/* do not wait for resources */
#define	M_ZERO		0x0002	/* zero the allocation */
#define	M_CANFAIL	0x0004	/* can fail if requested memory can't ever
				 * be allocated */
#include <sys/mallocvar.h>
/*
 * The following are standard, built-in malloc types that are
 * not specific to any one subsystem.
 */
MALLOC_DECLARE(M_DEVBUF);
MALLOC_DECLARE(M_DMAMAP);
MALLOC_DECLARE(M_FREE);
MALLOC_DECLARE(M_PCB);
MALLOC_DECLARE(M_TEMP);

/* XXX These should all be declared elsewhere. */
MALLOC_DECLARE(M_RTABLE);
MALLOC_DECLARE(M_FTABLE);
MALLOC_DECLARE(M_UFSMNT);
MALLOC_DECLARE(M_NETADDR);
MALLOC_DECLARE(M_IPMOPTS);
MALLOC_DECLARE(M_IPMADDR);
MALLOC_DECLARE(M_MRTABLE);
MALLOC_DECLARE(M_BWMETER);
MALLOC_DECLARE(M_1394DATA);
#endif /* _KERNEL */

/*
 * Set of buckets for each size of memory block that is retained
 */
struct kmembuckets {
	void *kb_next;	/* list of free blocks */
	void *kb_last;	/* last free block */
	long	kb_calls;	/* total calls to allocate this size */
	long	kb_total;	/* total number of blocks allocated */
	long	kb_totalfree;	/* # of free elements in this bucket */
	long	kb_elmpercl;	/* # of elements in this sized allocation */
	long	kb_highwat;	/* high water mark */
	long	kb_couldfree;	/* over high water mark and could free */
};

#ifdef _KERNEL
#ifdef MALLOCLOG
void	*_kern_malloc(unsigned long, struct malloc_type *, int, const char *, long);
void	_kern_free(void *, struct malloc_type *, const char *, long);
#define	malloc(size, type, flags) \
	    _kern_malloc((size), (type), (flags), __FILE__, __LINE__)
#define	free(addr, type) \
	    _kern_free((addr), (type), __FILE__, __LINE__)
#else
void	*kern_malloc(unsigned long, struct malloc_type *, int);
void	kern_free(void *, struct malloc_type *);
#define malloc(size, type, flags) kern_malloc(size, type, flags)
#define free(addr, type) kern_free(addr, type)
#endif /* MALLOCLOG */

#ifdef MALLOC_DEBUG
void	debug_malloc_init(void);
int	debug_malloc(unsigned long, struct malloc_type *, int, void **);
int	debug_free(void *, struct malloc_type *);

void	debug_malloc_print(void);
void	debug_malloc_printit(void (*)(const char *, ...), vaddr_t);
#endif /* MALLOC_DEBUG */

void	*kern_realloc(void *, unsigned long, struct malloc_type *, int);
#define realloc(ptr, size, type, flags) \
	    kern_realloc(ptr, size, type, flags)
unsigned long
	malloc_roundup(unsigned long);
#endif /* _KERNEL */
#endif /* !_SYS_MALLOC_H_ */
