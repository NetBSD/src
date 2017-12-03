/*	$NetBSD: slab.h,v 1.4.4.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_SLAB_H_
#define _LINUX_SLAB_H_

#include <sys/kmem.h>
#include <sys/malloc.h>

#include <machine/limits.h>

#include <uvm/uvm_extern.h>	/* For PAGE_SIZE.  */

#include <linux/gfp.h>

/* XXX Should use kmem, but Linux kfree doesn't take the size.  */

static inline int
linux_gfp_to_malloc(gfp_t gfp)
{
	int flags = 0;

	/* This has no meaning to us.  */
	gfp &= ~__GFP_NOWARN;
	gfp &= ~__GFP_RECLAIMABLE;

	/* Pretend this was the same as not passing __GFP_WAIT.  */
	if (ISSET(gfp, __GFP_NORETRY)) {
		gfp &= ~__GFP_NORETRY;
		gfp &= ~__GFP_WAIT;
	}

	if (ISSET(gfp, __GFP_ZERO)) {
		flags |= M_ZERO;
		gfp &= ~__GFP_ZERO;
	}

	/*
	 * XXX Handle other cases as they arise -- prefer to fail early
	 * rather than allocate memory without respecting parameters we
	 * don't understand.
	 */
	KASSERT((gfp == GFP_ATOMIC) ||
	    ((gfp & ~__GFP_WAIT) == (GFP_KERNEL & ~__GFP_WAIT)));

	if (ISSET(gfp, __GFP_WAIT)) {
		flags |= M_WAITOK;
		gfp &= ~__GFP_WAIT;
	} else {
		flags |= M_NOWAIT;
	}

	return flags;
}

static inline void *
kmalloc(size_t size, gfp_t gfp)
{
	return malloc(size, M_TEMP, linux_gfp_to_malloc(gfp));
}

static inline void *
kzalloc(size_t size, gfp_t gfp)
{
	return malloc(size, M_TEMP, (linux_gfp_to_malloc(gfp) | M_ZERO));
}

static inline void *
kmalloc_array(size_t n, size_t size, gfp_t gfp)
{
	if ((size != 0) && (n > (SIZE_MAX / size)))
		return NULL;
	return malloc((n * size), M_TEMP, linux_gfp_to_malloc(gfp));
}

static inline void *
kcalloc(size_t n, size_t size, gfp_t gfp)
{
	return kmalloc_array(n, size, (gfp | __GFP_ZERO));
}

static inline void *
krealloc(void *ptr, size_t size, gfp_t gfp)
{
	return realloc(ptr, size, M_TEMP, linux_gfp_to_malloc(gfp));
}

static inline void
kfree(void *ptr)
{
	if (ptr != NULL)
		free(ptr, M_TEMP);
}

#define	SLAB_HWCACHE_ALIGN	1

struct kmem_cache {
	pool_cache_t	kc_pool_cache;
	size_t		kc_size;
	void		(*kc_ctor)(void *);
};

static int
kmem_cache_ctor(void *cookie, void *ptr, int flags __unused)
{
	struct kmem_cache *const kc = cookie;

	if (kc->kc_ctor)
		(*kc->kc_ctor)(ptr);

	return 0;
}

static inline struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t align,
    unsigned long flags, void (*ctor)(void *))
{
	struct kmem_cache *kc;

	if (ISSET(flags, SLAB_HWCACHE_ALIGN))
		align = roundup(MAX(1, align), CACHE_LINE_SIZE);

	kc = kmem_alloc(sizeof(*kc), KM_SLEEP);
	kc->kc_pool_cache = pool_cache_init(size, align, 0, 0, name, NULL,
	    IPL_NONE, &kmem_cache_ctor, NULL, kc);
	kc->kc_size = size;
	kc->kc_ctor = ctor;

	return kc;
}

static inline void
kmem_cache_destroy(struct kmem_cache *kc)
{

	pool_cache_destroy(kc->kc_pool_cache);
	kmem_free(kc, sizeof(*kc));
}

static inline void *
kmem_cache_alloc(struct kmem_cache *kc, gfp_t gfp)
{
	int flags = 0;
	void *ptr;

	if (gfp & __GFP_WAIT)
		flags |= PR_NOWAIT;
	else
		flags |= PR_WAITOK;

	ptr = pool_cache_get(kc->kc_pool_cache, flags);
	if (ptr == NULL)
		return NULL;

	if (ISSET(gfp, __GFP_ZERO))
		(void)memset(ptr, 0, kc->kc_size);

	return ptr;
}

static inline void *
kmem_cache_zalloc(struct kmem_cache *kc, gfp_t gfp)
{

	return kmem_cache_alloc(kc, (gfp | __GFP_ZERO));
}

static inline void
kmem_cache_free(struct kmem_cache *kc, void *ptr)
{

	pool_cache_put(kc->kc_pool_cache, ptr);
}

#endif  /* _LINUX_SLAB_H_ */
