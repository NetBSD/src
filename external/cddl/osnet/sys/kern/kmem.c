/*	$NetBSD: kmem.c,v 1.2.2.1 2022/08/03 15:54:23 martin Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
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

#include <sys/kmem.h>

struct kmem_cache {
	pool_cache_t km_pool;
	char km_name[32];
	void *km_private;
	int (*km_constructor)(void *, void *, int);
	void (*km_destructor)(void *, void *);
	void (*km_reclaim)(void *);
};

static int
solaris_constructor(void *private, void *object, int flag)
{
	kmem_cache_t *km = private;

	if (km->km_constructor)
		return (*km->km_constructor)(object, km->km_private, flag);

	return 0;
}

static void
solaris_destructor(void *private, void *object)
{
	kmem_cache_t *km = private;

	if (km->km_destructor)
		(*km->km_destructor)(object, km->km_private);
}

static void
solaris_reclaim(void *private, int flag)
{

	kmem_cache_t *km = private;

	if (km->km_reclaim)
		(*km->km_reclaim)(km->km_private);
}

kmem_cache_t *
kmem_cache_create(char *name, size_t bufsize, size_t align,
    int (*constructor)(void *, void *, int), void (*destructor)(void *, void *),
    void (*reclaim)(void *) __unused, void *private, vmem_t *vmp, int flags)
{
	kmem_cache_t *km;

	KASSERT(ISSET(flags, ~(KMC_NOTOUCH | KMC_NODEBUG)) == 0);
	KASSERT(private == NULL);
	KASSERT(vmp == NULL);

	km = kmem_zalloc(sizeof(*km), KM_SLEEP);
	strlcpy(km->km_name, name, sizeof(km->km_name));
	km->km_private = private;
	km->km_constructor = constructor;
	km->km_destructor = destructor;
	km->km_reclaim = reclaim;
	km->km_pool = pool_cache_init(bufsize, align, 0, 0, km->km_name, NULL,
	    IPL_NONE, solaris_constructor, solaris_destructor, km);
	if (km->km_pool == NULL) {
		kmem_free(km, sizeof(*km));
		return NULL;
	}
	if (reclaim)
		pool_cache_set_drain_hook(km->km_pool, solaris_reclaim, km);

	return km;
}
void
kmem_cache_destroy(kmem_cache_t *km)
{

	pool_cache_destroy(km->km_pool);
	kmem_free(km, sizeof(*km));
}

void *
kmem_cache_alloc(kmem_cache_t *km, int flags)
{

	KASSERT(ISSET(flags, ~(KM_SLEEP | KM_NOSLEEP | KM_PUSHPAGE)) == 0);

	return pool_cache_get(km->km_pool, flags);
}

void
kmem_cache_free(kmem_cache_t *km, void *object)
{

	pool_cache_put(km->km_pool, object);
}

void
kmem_cache_reap_now(kmem_cache_t *km)
{

	pool_cache_invalidate(km->km_pool);
}

#undef kmem_alloc
#undef kmem_zalloc
#undef kmem_free

/*
 * Solaris allows allocating zero bytes (which returns NULL)
 * and freeing a NULL pointer (which does nothing),
 * so allow that here with wrappers around the native functions
 * (which do not allow those things).
 */

void *
solaris_kmem_alloc(size_t size, int flags)
{

	if (size == 0)
		return NULL;
	return kmem_alloc(size, flags);
}

void *
solaris_kmem_zalloc(size_t size, int flags)
{

	if (size == 0)
		return NULL;
	return kmem_zalloc(size, flags);
}

void
solaris_kmem_free(void *buf, size_t size)
{

	if (buf == NULL)
		return;
	kmem_free(buf, size);
}
