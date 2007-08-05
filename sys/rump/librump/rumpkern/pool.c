/*	$NetBSD: pool.c,v 1.1 2007/08/05 22:28:09 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/pool.h>

#include "rumpuser.h"

struct pool_cache pnbuf_cache;
struct pool pnbuf_pool;
struct pool_allocator pool_allocator_nointr;

void
pool_init(struct pool *pp, size_t size, u_int align, u_int align_offset,
	int flags, const char *wchan, struct pool_allocator *palloc, int ipl)
{

	pp->pr_size = size;
}

void
pool_destroy(struct pool *pp)
{

	return;
}

void
pool_cache_init(struct pool_cache *pc, struct pool *pp,
	int (*ctor)(void *, void *, int), void (*dtor)(void *, void *),
	void *arg)
{

	pc->pc_pool = pp;
	pc->pc_ctor = ctor;
	pc->pc_dtor = dtor;
	pc->pc_arg = arg;
}

void *
pool_cache_get_paddr(struct pool_cache *pc, int flags, paddr_t *pap)
{
	void *item;

	item = pool_get(pc->pc_pool, 0);
	if (pc->pc_ctor)
		pc->pc_ctor(pc->pc_arg, item, flags);
	if (pap)
		*pap = POOL_PADDR_INVALID;

	return item;
}

void
pool_cache_put_paddr(struct pool_cache *pc, void *object, paddr_t pa)
{

	if (pc->pc_dtor)
		pc->pc_dtor(pc->pc_arg, object);
	pool_put(pc->pc_pool, object);
}

void *
pool_get(struct pool *pp, int flags)
{
	void *rv;

	if (pp->pr_size == 0)
		panic("%s: pool unit size 0.  not initialized?", __func__);

	rv = rumpuser_malloc(pp->pr_size, 1);
	if (rv == NULL && (flags & PR_WAITOK && (flags & PR_LIMITFAIL) == 0))
		panic("%s: out of memory and PR_WAITOK", __func__);

	return rv;
}

void
pool_put(struct pool *pp, void *item)
{

	rumpuser_free(item);
}

/* XXX: for tmpfs, shouldn't be here */
void *pool_page_alloc_nointr(struct pool *, int);
void pool_page_free_nointr(struct pool *, void *);
void *
pool_page_alloc_nointr(struct pool *pp, int flags)
{

	return pool_get(pp, flags);
}

void
pool_page_free_nointr(struct pool *pp, void *item)
{

	return pool_put(pp, item);
}
