/*	$NetBSD: memalloc.c,v 1.11.8.1 2012/04/17 00:08:49 yamt Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: memalloc.c,v 1.11.8.1 2012/04/17 00:08:49 yamt Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/percpu.h>
#include <sys/pool.h>
#include <sys/vmem.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * Allocator "implementations" which relegate tasks to the host
 * libc malloc.
 *
 * Supported:
 *   + malloc
 *   + kmem
 *   + pool
 *   + pool_cache
 */

/*
 * malloc
 */

void
malloc_type_attach(struct malloc_type *type)
{

	return;
}

void
malloc_type_detach(struct malloc_type *type)
{

	return;
}

void *
kern_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	void *rv;

	rv = rumpuser_malloc(size, 0);

	if (__predict_false(rv == NULL && (flags & (M_CANFAIL|M_NOWAIT)) == 0))
		panic("malloc %lu bytes failed", size);

	if (rv && flags & M_ZERO)
		memset(rv, 0, size);

	return rv;
}

void *
kern_realloc(void *ptr, unsigned long size, struct malloc_type *type, int flags)
{

	return rumpuser_realloc(ptr, size);
}

void
kern_free(void *ptr, struct malloc_type *type)
{

	rumpuser_free(ptr);
}

/*
 * Kmem
 */

#ifdef RUMP_USE_UNREAL_ALLOCATORS
void
kmem_init()
{

	/* nothing to do */
}

void *
kmem_alloc(size_t size, km_flag_t kmflag)
{

	return rump_hypermalloc(size, 0, kmflag == KM_SLEEP, "kmem_alloc");
}

void *
kmem_zalloc(size_t size, km_flag_t kmflag)
{
	void *rv;

	rv = kmem_alloc(size, kmflag);
	if (rv)
		memset(rv, 0, size);

	return rv;
}

void
kmem_free(void *p, size_t size)
{

	rumpuser_free(p);
}

/*
 * pool & pool_cache
 */

struct pool_cache pnbuf_cache;
struct pool pnbuf_pool;
struct pool_allocator pool_allocator_nointr;

void
pool_subsystem_init()
{

	/* nada */
}

void
pool_init(struct pool *pp, size_t size, u_int align, u_int align_offset,
	int flags, const char *wchan, struct pool_allocator *palloc, int ipl)
{

	pp->pr_size = size;
	pp->pr_align = align;
}

void
pool_destroy(struct pool *pp)
{

	return;
}

pool_cache_t
pool_cache_init(size_t size, u_int align, u_int align_offset, u_int flags,
	const char *wchan, struct pool_allocator *palloc, int ipl,
	int (*ctor)(void *, void *, int), void (*dtor)(void *, void *),
	void *arg)
{
	pool_cache_t pc;

	pc = rump_hypermalloc(sizeof(*pc), 0, true, "pcinit");
	pool_cache_bootstrap(pc, size, align, align_offset, flags, wchan,
	    palloc, ipl, ctor, dtor, arg);
	return pc;
}

void
pool_cache_bootstrap(pool_cache_t pc, size_t size, u_int align,
	u_int align_offset, u_int flags, const char *wchan,
	struct pool_allocator *palloc, int ipl,
	int (*ctor)(void *, void *, int), void (*dtor)(void *, void *),
	void *arg)
{

	pool_init(&pc->pc_pool, size, align, align_offset, flags,
	    wchan, palloc, ipl);
	pc->pc_ctor = ctor;
	pc->pc_dtor = dtor;
	pc->pc_arg = arg;
}

void
pool_cache_destroy(pool_cache_t pc)
{

	pool_destroy(&pc->pc_pool);
	rumpuser_free(pc);
}

void *
pool_cache_get_paddr(pool_cache_t pc, int flags, paddr_t *pap)
{
	void *item;

	item = pool_get(&pc->pc_pool, 0);
	if (pc->pc_ctor)
		pc->pc_ctor(pc->pc_arg, item, flags);
	if (pap)
		*pap = POOL_PADDR_INVALID;

	return item;
}

void
pool_cache_put_paddr(pool_cache_t pc, void *object, paddr_t pa)
{

	if (pc->pc_dtor)
		pc->pc_dtor(pc->pc_arg, object);
	pool_put(&pc->pc_pool, object);
}

bool
pool_cache_reclaim(pool_cache_t pc)
{

	return true;
}

void
pool_cache_cpu_init(struct cpu_info *ci)
{

	return;
}

void *
pool_get(struct pool *pp, int flags)
{

#ifdef DIAGNOSTIC
	if (pp->pr_size == 0)
		panic("%s: pool unit size 0.  not initialized?", __func__);
#endif

	return rump_hypermalloc(pp->pr_size, pp->pr_align,
	    (flags & PR_WAITOK) != 0, "pget");
}

void
pool_put(struct pool *pp, void *item)
{

	rumpuser_free(item);
}

void
pool_sethiwat(struct pool *pp, int n)
{

	return;
}

void
pool_setlowat(struct pool *pp, int n)
{

	return;
}

void
pool_cache_sethardlimit(pool_cache_t pc, int n, const char *warnmess,
	int ratecap)
{

	return;
}

void
pool_cache_setlowat(pool_cache_t pc, int n)
{

	return;
}

void
pool_cache_set_drain_hook(pool_cache_t pc, void (*fn)(void *, int), void *arg)
{

	/* XXX: notused */
	pc->pc_pool.pr_drain_hook = fn;
	pc->pc_pool.pr_drain_hook_arg = arg;
}

void
pool_drain_start(struct pool **ppp, uint64_t *wp)
{

	/* nada */
}

bool
pool_drain_end(struct pool *pp, uint64_t w)
{

	/* can't reclaim anything in this model */
	return false;
}

int
pool_prime(struct pool *pp, int nitems)
{

	return 0;
}

/* XXX: for tmpfs, shouldn't be here */
void *pool_page_alloc(struct pool *, int);
void pool_page_free(struct pool *, void *);
void *
pool_page_alloc(struct pool *pp, int flags)
{

	return pool_get(pp, flags);
}

void
pool_page_free(struct pool *pp, void *item)
{

	return pool_put(pp, item);
}

void
vmem_rehash_start()
{

	return;
}

/*
 * A simplified percpu is included in here since subr_percpu.c uses
 * the vmem allocator and I don't want to reimplement vmem.  So use
 * this simplified percpu for non-vmem systems.
 */

static kmutex_t pcmtx;

void
percpu_init(void)
{

	mutex_init(&pcmtx, MUTEX_DEFAULT, IPL_NONE);
}

void
percpu_init_cpu(struct cpu_info *ci)
{

	/* nada */
}

void *
percpu_getref(percpu_t *pc)
{

	mutex_enter(&pcmtx);
	return pc;
}

void
percpu_putref(percpu_t *pc)
{

	mutex_exit(&pcmtx);
}

percpu_t *
percpu_alloc(size_t size)
{

	return kmem_alloc(size, KM_SLEEP);
}

void
percpu_free(percpu_t *pc, size_t size)
{

	kmem_free(pc, size);
}

void
percpu_foreach(percpu_t *pc, percpu_callback_t cb, void *arg)
{

	cb(pc, arg, rump_cpu);
}
#endif /* RUMP_USE_UNREAL_ALLOCATORS */
