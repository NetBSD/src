
/*	$NetBSD: kmem.h,v 1.9.2.1 2018/06/25 07:25:26 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _OPENSOLARIS_SYS_KMEM_H_
#define	_OPENSOLARIS_SYS_KMEM_H_

#include_next <sys/kmem.h>
#include <sys/pool.h>
#include <sys/vmem.h>

#define	KM_PUSHPAGE	KM_SLEEP
#define	KM_NORMALPRI	0
#define	KM_NODEBUG	0

#define	KMC_NODEBUG	0
#define	KMC_NOTOUCH	0

typedef void kmem_cache_t;

#define	POINTER_IS_VALID(p)	(!((uintptr_t)(p) & 0x3))
#define	POINTER_INVALIDATE(pp)	(*(pp) = (void *)((uintptr_t)(*(pp)) | 0x1))

void		kmem_reap(void);

static inline kmem_cache_t *
kmem_cache_create(char *name, size_t bufsize, size_t align,
    int (*constructor)(void *, void *, int), void (*destructor)(void *, void *),
    void (*reclaim)(void *) __unused, void *private, vmem_t *vmp, int cflags)
{
	pool_cache_t pc;
	int flags = bufsize > PAGESIZE ? PR_NOALIGN : 0;

	KASSERT(vmp == NULL);

	pc = pool_cache_init(bufsize, align, 0, flags, name, NULL, IPL_NONE,
	    constructor, destructor, private);
	if (pc != NULL && reclaim != NULL) {
		pool_cache_set_drain_hook(pc, (void *)reclaim, private);
	}
	return pc;
}

static inline void *
kmem_cache_alloc(kmem_cache_t *cache, int flags)
{
	return pool_cache_get(cache, flags);
}

#define	kmem_cache_destroy(cache)		pool_cache_destroy(cache)
#define	kmem_cache_free(cache, buf)		pool_cache_put(cache, buf)
#define	kmem_cache_reap_now(cache)		pool_cache_invalidate(cache)

#define	heap_arena			kmem_arena

#define	kmem_cache_set_move(cache, movefunc)	do { } while (0)

#define kmem_alloc solaris_kmem_alloc
#define kmem_zalloc solaris_kmem_zalloc
#define kmem_free solaris_kmem_free
#define kmem_size() ((uint64_t)physmem * PAGE_SIZE)

void *solaris_kmem_alloc(size_t, int);
void *solaris_kmem_zalloc(size_t, int);
void solaris_kmem_free(void *, size_t);

#endif	/* _OPENSOLARIS_SYS_KMEM_H_ */
