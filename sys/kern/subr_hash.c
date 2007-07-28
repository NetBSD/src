/*	$NetBSD: subr_hash.c,v 1.1 2007/07/28 12:53:52 pooka Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_subr.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_hash.c,v 1.1 2007/07/28 12:53:52 pooka Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>

/*
 * General routine to allocate a hash table.
 * Allocate enough memory to hold at least `elements' list-head pointers.
 * Return a pointer to the allocated space and set *hashmask to a pattern
 * suitable for masking a value to use as an index into the returned array.
 */
void *
hashinit(u_int elements, enum hashtype htype, struct malloc_type *mtype,
    int mflags, u_long *hashmask)
{
	u_long hashsize, i;
	LIST_HEAD(, generic) *hashtbl_list;
	TAILQ_HEAD(, generic) *hashtbl_tailq;
	size_t esize;
	void *p;

	if (elements == 0)
		panic("hashinit: bad cnt");
	for (hashsize = 1; hashsize < elements; hashsize <<= 1)
		continue;

	switch (htype) {
	case HASH_LIST:
		esize = sizeof(*hashtbl_list);
		break;
	case HASH_TAILQ:
		esize = sizeof(*hashtbl_tailq);
		break;
	default:
#ifdef DIAGNOSTIC
		panic("hashinit: invalid table type");
#else
		return NULL;
#endif
	}

	if ((p = malloc(hashsize * esize, mtype, mflags)) == NULL)
		return (NULL);

	switch (htype) {
	case HASH_LIST:
		hashtbl_list = p;
		for (i = 0; i < hashsize; i++)
			LIST_INIT(&hashtbl_list[i]);
		break;
	case HASH_TAILQ:
		hashtbl_tailq = p;
		for (i = 0; i < hashsize; i++)
			TAILQ_INIT(&hashtbl_tailq[i]);
		break;
	}
	*hashmask = hashsize - 1;
	return (p);
}

/*
 * Free memory from hash table previosly allocated via hashinit().
 */
void
hashdone(void *hashtbl, struct malloc_type *mtype)
{

	free(hashtbl, mtype);
}
