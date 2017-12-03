/*	$NetBSD: alloc.c,v 1.26.12.1 2017/12/03 11:38:46 jdolecek Exp $	*/

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)alloc.c	8.1 (Berkeley) 6/11/93
 *
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *
 *	@(#)alloc.c	8.1 (Berkeley) 6/11/93
 *
 *
 * Copyright (c) 1989, 1990, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Author: Alessandro Forin
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Dynamic memory allocator.
 *
 * Compile options:
 *
 *	HEAP_LIMIT	heap limit address (defaults to "no limit").
 *
 *	HEAP_START	start address of heap (defaults to '&end').
 *
 *	DEBUG		enable debugging sanity checks.
 */

#include <sys/param.h>
#include "stand.h"

/*
 * Each block actually has ALIGN(unsigned int) + ALIGN(size) bytes allocated
 * to it, as follows:
 *
 * 0 ... (sizeof(unsigned int) - 1)
 *	allocated or unallocated: holds size of user-data part of block.
 *
 * sizeof(unsigned int) ... (ALIGN(sizeof(unsigned int)) - 1)
 *	allocated: unused
 *	unallocated: depends on packing of struct fl
 *
 * ALIGN(sizeof(unsigned int)) ...
 *     (ALIGN(sizeof(unsigned int)) + ALIGN(data size) - 1)
 *	allocated: user data
 *	unallocated: depends on packing of struct fl
 *
 * 'next' is only used when the block is unallocated (i.e. on the free list).
 * However, note that ALIGN(sizeof(unsigned int)) + ALIGN(data size) must
 * be at least 'sizeof(struct fl)', so that blocks can be used as structures
 * when on the free list.
 *
 * When HEAP_LIMIT is defined and the heap limit is reached, alloc() panics.
 * Otherwise, it never fails.
 */
struct fl {
	unsigned int	size;
	struct fl	*next;
} *freelist;

#ifdef HEAP_VARIABLE
static char *top, *heapstart, *heaplimit;
void
setheap(void *start, void *limit)
{
	heapstart = top = start;
	heaplimit = limit;
}
#define HEAP_START heapstart
#define HEAP_LIMIT heaplimit
#else /* !HEAP_VARIABLE */
#ifndef HEAP_START
extern char end[];
#define HEAP_START end
#endif
static char *top = (char *)HEAP_START;
#endif /* HEAP_VARIABLE */

__compactcall void *
alloc(size_t size)
{
	struct fl **f = &freelist, **bestf = NULL;
	unsigned int bestsize = 0xffffffff;	/* greater than any real size */
	char *help;
	int failed;

	/* scan freelist */
	while (*f) {
		if ((size_t)(*f)->size >= size) {
			if ((size_t)(*f)->size == size) /* exact match */
				goto found;

			if ((*f)->size < bestsize) {
				/* keep best fit */
				bestf = f;
				bestsize = (*f)->size;
			}
		}
		f = &((*f)->next);
	}

	/* no match in freelist if bestsize unchanged */
	failed = (bestsize == 0xffffffff);

	if (failed) { /* nothing found */
		/*
		 * allocate from heap, keep chunk len in
		 * first word
		 */
		help = top;

		/* make _sure_ the region can hold a struct fl. */
		if (size < ALIGN(sizeof (struct fl *)))
			size = ALIGN(sizeof (struct fl *));
		top += ALIGN(sizeof(unsigned int)) + ALIGN(size);
#ifdef HEAP_LIMIT
		if (top > (char *)HEAP_LIMIT)
			panic("heap full (%p+%zu)", help, size);
#endif
		*(unsigned int *)(void *)help = (unsigned int)ALIGN(size);
		return help + ALIGN(sizeof(unsigned int));
	}

	/* we take the best fit */
	f = bestf;

found:
	/* remove from freelist */
	help = (char *)(void *)*f;
	*f = (*f)->next;
	return help + ALIGN(sizeof(unsigned int));
}

__compactcall void
/*ARGSUSED*/
dealloc(void *ptr, size_t size)
{
	struct fl *f =
	    (struct fl *)(void *)((char *)(void *)ptr -
	    ALIGN(sizeof(unsigned int)));
#ifdef DEBUG
	if (size > (size_t)f->size) {
		printf("dealloc %zu bytes @%lx, should be <=%u\n",
			size, (u_long)ptr, f->size);
	}

	if (ptr < (void *)HEAP_START)
		printf("dealloc: %lx before start of heap.\n", (u_long)ptr);

#ifdef HEAP_LIMIT
	if (ptr > (void *)HEAP_LIMIT)
		printf("dealloc: %lx beyond end of heap.\n", (u_long)ptr);
#endif
#endif /* DEBUG */
	/* put into freelist */
	f->next = freelist;
	freelist = f;
}
