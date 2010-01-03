
/*	$NetBSD: kmem.h,v 1.3 2010/01/03 11:33:13 ober Exp $	*/

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
#include_next <sys/pool.h>
#include_next <sys/vmem.h>

typedef void kmem_cache_t;

u_long	kmem_size(void);
u_long	kmem_used(void);
void	kmem_reap(void);

void	*calloc(size_t n, size_t s);

static inline kmem_cache_t *
kmem_cache_create(char *name, size_t bufsize, size_t align,
    int (*constructor)(void *, void *, int), void (*destructor)(void *, void *),
    void (*reclaim)(void *) __unused, void *private, vmem_t *vmp, int cflags)
{
	pool_cache_t pc;

	KASSERT(vmp == NULL);

	pc = pool_cache_init(bufsize, align, 0, 0, name, NULL, IPL_NONE,
	    constructor, destructor, private);
	if (pc != NULL && reclaim != NULL) {
		pool_cache_set_drain_hook(pc, (void *)reclaim, private);
	}
	return pc;
}

#define	kmem_cache_destroy(cache)		pool_cache_destroy(cache)
#define	kmem_cache_alloc(cache, flags)		pool_cache_get(cache, flags)
#define	kmem_cache_free(cache, buf)		pool_cache_put(cache, buf)
#define	kmem_cache_reap_now(cache)		pool_cache_invalidate(cache)

#define	KM_PUSHPAGE	KM_SLEEP	/* XXXNETBSD XXX HACK to prevent the crashes currently seen. Should be revisited once the uvm issues with zfs are fixed. */
#define	KMC_NODEBUG	0x00

#endif	/* _OPENSOLARIS_SYS_KMEM_H_ */
