/*	$KAME: gcmalloc.h,v 1.2 2001/04/03 15:51:55 thorpej Exp $	*/

/*
 * Copyright (C) 2000, 2001 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Debugging malloc glue for Racoon.
 */

#ifndef _GCMALLOC_H_DEFINED
#define _GCMALLOC_H_DEFINED

/* ElectricFence needs no special handling. */

/*
 * Boehm-GC provides GC_malloc(), GC_realloc(), GC_free() functions,
 * but not the traditional entry points.  So what we do is provide  
 * malloc(), calloc(), realloc(), and free() entry points in the main
 * program and letting the linker do the rest.
 */
#ifdef GC
#define GC_DEBUG
#include <gc.h>

#ifdef RACOON_MAIN_PROGRAM
void *
malloc(size_t size)
{

	return (GC_MALLOC(size));
}

void *
calloc(size_t number, size_t size)
{

	/* GC_malloc() clears the storage. */
	return (GC_MALLOC(number * size));
}

void *
realloc(void *ptr, size_t size)
{

	return (GC_REALLOC(ptr, size));
}

void
free(void *ptr)
{

	GC_FREE(ptr);
}
#endif /* RACOON_MAIN_PROGRAM */

#define	racoon_malloc(sz)	GC_debug_malloc(sz, GC_EXTRAS)
#define	racoon_calloc(cnt, sz)	GC_debug_malloc(cnt * sz, GC_EXTRAS)
#define	racoon_realloc(old, sz)	GC_debug_realloc(old, sz, GC_EXTRAS)
#define	racoon_free(p)		GC_debug_free(p)

#endif /* GC */

#ifndef racoon_malloc
#define	racoon_malloc(sz)	malloc((sz))
#endif
#ifndef racoon_calloc
#define	racoon_calloc(cnt, sz)	calloc((cnt), (sz))
#endif
#ifndef racoon_realloc
#define	racoon_realloc(old, sz)	realloc((old), (sz))
#endif
#ifndef racoon_free
#define	racoon_free(p)		free((p))
#endif

#endif /* _GCMALLOC_H_DEFINED */
