/*	$NetBSD: alloc.c,v 1.4.28.1 2011/06/06 09:06:48 jruoho Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Dynamic memory allocator suitable for use with OpenFirmware.
 *
 * Compile options:
 *
 *	ALLOC_TRACE	enable tracing of allocations/deallocations
 *
 *	ALLOC_FIRST_FIT	use a first-fit allocation algorithm, rather than
 *			the default best-fit algorithm.
 *
 *	DEBUG		enable debugging sanity checks.
 */

#include <sys/param.h>
#include <sys/queue.h>

#include <lib/libsa/stand.h>

#include "openfirm.h"
#include "boot.h"

/*
 * Each block actually has ALIGN(struct ml) + ALIGN(size) bytes allocated
 * to it, as follows:
 *
 * 0 ... (sizeof(struct ml) - 1)
 *	allocated or unallocated: holds size of user-data part of block.
 *
 * sizeof(struct ml) ... (ALIGN(sizeof(struct ml)) - 1)
 *	allocated: unused
 *	unallocated: depends on packing of struct fl
 *
 * ALIGN(sizeof(struct ml)) ... (ALIGN(sizeof(struct ml)) +
 *   ALIGN(data size) - 1)
 *	allocated: user data
 *	unallocated: depends on packing of struct fl
 *
 * 'next' is only used when the block is unallocated (i.e. on the free list).
 * However, note that ALIGN(sizeof(struct ml)) + ALIGN(data size) must
 * be at least 'sizeof(struct fl)', so that blocks can be used as structures
 * when on the free list.
 */

/*
 * Memory lists.
 */
struct ml {
	unsigned	size;
	LIST_ENTRY(ml)	list;
};

LIST_HEAD(, ml) freelist = LIST_HEAD_INITIALIZER(freelist);
LIST_HEAD(, ml) allocatedlist = LIST_HEAD_INITIALIZER(allocatedlist);

#define	OVERHEAD	ALIGN(sizeof (struct ml))	/* shorthand */

void *
alloc(size_t size)
{
	struct ml *f, *bestf;
#ifndef ALLOC_FIRST_FIT
	unsigned bestsize = 0xffffffff;	/* greater than any real size */
#endif
	char *help;
	int failed;

#ifdef ALLOC_TRACE
	printf("alloc(%zu)", size);
#endif

	/*
	 * Account for overhead now, so that we don't get an
	 * "exact fit" which doesn't have enough space.
	 */
	size = ALIGN(size) + OVERHEAD;

#ifdef ALLOC_FIRST_FIT
	/* scan freelist */
	for (f = freelist.lh_first; f != NULL && (size_t)f->size < size;
	    f = f->list.le_next)
		/* noop */ ;
	bestf = f;
	failed = (bestf == NULL);
#else
	/* scan freelist */
	bestf = NULL;		/* XXXGCC: -Wuninitialized */
	f = freelist.lh_first;
	while (f != NULL) {
		if ((size_t)f->size >= size) {
			if ((size_t)f->size == size)	/* exact match */
				goto found;

			if (f->size < bestsize) {
				/* keep best fit */
				bestf = f;
				bestsize = f->size;
			}
		}
		f = f->list.le_next;
	}

	/* no match in freelist if bestsize unchanged */
	failed = (bestsize == 0xffffffff);
#endif

	if (failed) {	/* nothing found */
		/*
		 * Allocate memory from the OpenFirmware, rounded
		 * to page size, and record the chunk size.
		 */
		size = roundup(size, NBPG);
		help = OF_claim(NULL, (unsigned)size, NBPG);
		if (help == (char *)-1)
			panic("alloc: out of memory");

		f = (struct ml *)help;
		f->size = (unsigned)size;
#ifdef ALLOC_TRACE
		printf("=%lx (new chunk size %u)\n",
		    (u_long)(help + OVERHEAD), f->size);
#endif
		goto out;
	}

	/* we take the best fit */
	f = bestf;

#ifndef ALLOC_FIRST_FIT
 found:
#endif
	/* remove from freelist */
	LIST_REMOVE(f, list);
	help = (char *)f;
#ifdef ALLOC_TRACE
	printf("=%lx (origsize %u)\n", (u_long)(help + OVERHEAD), f->size);
#endif
 out:
	/* place on allocated list */
	LIST_INSERT_HEAD(&allocatedlist, f, list);
	return (help + OVERHEAD);
}

void
dealloc(void *ptr, size_t size)
{
	register struct ml *a = (struct ml *)((char*)ptr - OVERHEAD);

#ifdef ALLOC_TRACE
	printf("dealloc(%lx, %zu) (origsize %u)\n", (u_long)ptr, size, a->size);
#endif
#ifdef DEBUG
	if (size > (size_t)a->size)
		printf("dealloc %zu bytes @%lx, should be <=%u\n",
		    size, (u_long)ptr, a->size);
#endif

	/* Remove from allocated list, place on freelist. */
	LIST_REMOVE(a, list);
	LIST_INSERT_HEAD(&freelist, a, list);
}

void
freeall(void)
{
#ifdef __notyet__		/* Firmware bug ?! */
	struct ml *m;

	/* Release chunks on freelist... */
	while ((m = freelist.lh_first) != NULL) {
		LIST_REMOVE(m, list);
		OF_release(m, m->size);
	}

	/* ...and allocated list. */
	while ((m = allocatedlist.lh_first) != NULL) {
		LIST_REMOVE(m, list);
		OF_release(m, m->size);
	}
#endif /* __notyet__ */
}
