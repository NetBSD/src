/*	$NetBSD: subr_kmem.c,v 1.32 2010/01/31 11:54:32 skrll Exp $	*/

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

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * allocator of kernel wired memory.
 *
 * TODO:
 * -	worth to have "intrsafe" version?  maybe..
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kmem.c,v 1.32 2010/01/31 11:54:32 skrll Exp $");

#include <sys/param.h>
#include <sys/callback.h>
#include <sys/kmem.h>
#include <sys/vmem.h>
#include <sys/debug.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm_kmguard.h>

#include <lib/libkern/libkern.h>

#define	KMEM_QUANTUM_SIZE	(ALIGNBYTES + 1)
#define	KMEM_QCACHE_MAX		(KMEM_QUANTUM_SIZE * 32)
#define	KMEM_CACHE_COUNT	16

typedef struct kmem_cache {
	pool_cache_t		kc_cache;
	struct pool_allocator	kc_pa;
	char			kc_name[12];
} kmem_cache_t;

static vmem_t *kmem_arena;
static struct callback_entry kmem_kva_reclaim_entry;

static kmem_cache_t kmem_cache[KMEM_CACHE_COUNT + 1];
static size_t kmem_cache_max;
static size_t kmem_cache_min;
static size_t kmem_cache_mask;
static int kmem_cache_shift;

#if defined(DEBUG)
int kmem_guard_depth;
size_t kmem_guard_size;
static struct uvm_kmguard kmem_guard;
static void *kmem_freecheck;
#define	KMEM_POISON
#define	KMEM_REDZONE
#define	KMEM_SIZE
#define	KMEM_GUARD
#endif /* defined(DEBUG) */

#if defined(KMEM_POISON)
static void kmem_poison_fill(void *, size_t);
static void kmem_poison_check(void *, size_t);
#else /* defined(KMEM_POISON) */
#define	kmem_poison_fill(p, sz)		/* nothing */
#define	kmem_poison_check(p, sz)	/* nothing */
#endif /* defined(KMEM_POISON) */

#if defined(KMEM_REDZONE)
#define	REDZONE_SIZE	1
#else /* defined(KMEM_REDZONE) */
#define	REDZONE_SIZE	0
#endif /* defined(KMEM_REDZONE) */

#if defined(KMEM_SIZE)
#define	SIZE_SIZE	(max(KMEM_QUANTUM_SIZE, sizeof(size_t)))
static void kmem_size_set(void *, size_t);
static void kmem_size_check(const void *, size_t);
#else
#define	SIZE_SIZE	0
#define	kmem_size_set(p, sz)	/* nothing */
#define	kmem_size_check(p, sz)	/* nothing */
#endif

static vmem_addr_t kmem_backend_alloc(vmem_t *, vmem_size_t, vmem_size_t *,
    vm_flag_t);
static void kmem_backend_free(vmem_t *, vmem_addr_t, vmem_size_t);
static int kmem_kva_reclaim_callback(struct callback_entry *, void *, void *);

CTASSERT(KM_SLEEP == PR_WAITOK);
CTASSERT(KM_NOSLEEP == PR_NOWAIT);

static inline vm_flag_t
kmf_to_vmf(km_flag_t kmflags)
{
	vm_flag_t vmflags;

	KASSERT((kmflags & (KM_SLEEP|KM_NOSLEEP)) != 0);
	KASSERT((~kmflags & (KM_SLEEP|KM_NOSLEEP)) != 0);

	vmflags = 0;
	if ((kmflags & KM_SLEEP) != 0) {
		vmflags |= VM_SLEEP;
	}
	if ((kmflags & KM_NOSLEEP) != 0) {
		vmflags |= VM_NOSLEEP;
	}

	return vmflags;
}

static void *
kmem_poolpage_alloc(struct pool *pool, int prflags)
{

	return (void *)vmem_alloc(kmem_arena, pool->pr_alloc->pa_pagesz,
	    kmf_to_vmf(prflags) | VM_INSTANTFIT);

}

static void
kmem_poolpage_free(struct pool *pool, void *addr)
{

	vmem_free(kmem_arena, (vmem_addr_t)addr, pool->pr_alloc->pa_pagesz);
}

/* ---- kmem API */

/*
 * kmem_alloc: allocate wired memory.
 *
 * => must not be called from interrupt context.
 */

void *
kmem_alloc(size_t size, km_flag_t kmflags)
{
	kmem_cache_t *kc;
	uint8_t *p;

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	KASSERT(size > 0);

#ifdef KMEM_GUARD
	if (size <= kmem_guard_size) {
		return uvm_kmguard_alloc(&kmem_guard, size,
		    (kmflags & KM_SLEEP) != 0);
	}
#endif

	size += REDZONE_SIZE + SIZE_SIZE;
	if (size >= kmem_cache_min && size <= kmem_cache_max) {
		kc = &kmem_cache[(size + kmem_cache_mask) >> kmem_cache_shift];
		KASSERT(size <= kc->kc_pa.pa_pagesz);
		kmflags &= (KM_SLEEP | KM_NOSLEEP);
		p = pool_cache_get(kc->kc_cache, kmflags);
	} else {
		p = (void *)vmem_alloc(kmem_arena, size,
		    kmf_to_vmf(kmflags) | VM_INSTANTFIT);
	}
	if (__predict_true(p != NULL)) {
		kmem_poison_check(p, kmem_roundup_size(size));
		FREECHECK_OUT(&kmem_freecheck, p);
		kmem_size_set(p, size);
		p = (uint8_t *)p + SIZE_SIZE;
	}
	return p;
}

/*
 * kmem_zalloc: allocate wired memory.
 *
 * => must not be called from interrupt context.
 */

void *
kmem_zalloc(size_t size, km_flag_t kmflags)
{
	void *p;

	p = kmem_alloc(size, kmflags);
	if (p != NULL) {
		memset(p, 0, size);
	}
	return p;
}

/*
 * kmem_free: free wired memory allocated by kmem_alloc.
 *
 * => must not be called from interrupt context.
 */

void
kmem_free(void *p, size_t size)
{
	kmem_cache_t *kc;

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	KASSERT(p != NULL);
	KASSERT(size > 0);

#ifdef KMEM_GUARD
	if (size <= kmem_guard_size) {
		uvm_kmguard_free(&kmem_guard, size, p);
		return;
	}
#endif
	size += SIZE_SIZE;
	p = (uint8_t *)p - SIZE_SIZE;
	kmem_size_check(p, size + REDZONE_SIZE);
	FREECHECK_IN(&kmem_freecheck, p);
	LOCKDEBUG_MEM_CHECK(p, size);
	kmem_poison_check((char *)p + size,
	    kmem_roundup_size(size + REDZONE_SIZE) - size);
	kmem_poison_fill(p, size);
	size += REDZONE_SIZE;
	if (size >= kmem_cache_min && size <= kmem_cache_max) {
		kc = &kmem_cache[(size + kmem_cache_mask) >> kmem_cache_shift];
		KASSERT(size <= kc->kc_pa.pa_pagesz);
		pool_cache_put(kc->kc_cache, p);
	} else {
		vmem_free(kmem_arena, (vmem_addr_t)p, size);
	}
}


void
kmem_init(void)
{
	kmem_cache_t *kc;
	size_t sz;
	int i;

#ifdef KMEM_GUARD
	uvm_kmguard_init(&kmem_guard, &kmem_guard_depth, &kmem_guard_size,
	    kernel_map);
#endif

	kmem_arena = vmem_create("kmem", 0, 0, KMEM_QUANTUM_SIZE,
	    kmem_backend_alloc, kmem_backend_free, NULL, KMEM_QCACHE_MAX,
	    VM_SLEEP, IPL_NONE);
	callback_register(&vm_map_to_kernel(kernel_map)->vmk_reclaim_callback,
	    &kmem_kva_reclaim_entry, kmem_arena, kmem_kva_reclaim_callback);

	/*
	 * kmem caches start at twice the size of the largest vmem qcache
	 * and end at PAGE_SIZE or earlier.  assert that KMEM_QCACHE_MAX
	 * is a power of two.
	 */
	KASSERT(ffs(KMEM_QCACHE_MAX) != 0);
	KASSERT(KMEM_QCACHE_MAX - (1 << (ffs(KMEM_QCACHE_MAX) - 1)) == 0);
	kmem_cache_shift = ffs(KMEM_QCACHE_MAX);
	kmem_cache_min = 1 << kmem_cache_shift;
	kmem_cache_mask = kmem_cache_min - 1;
	for (i = 1; i <= KMEM_CACHE_COUNT; i++) {
		sz = i << kmem_cache_shift;
		if (sz > PAGE_SIZE) {
			break;
		}
		kmem_cache_max = sz;
		kc = &kmem_cache[i];
		kc->kc_pa.pa_pagesz = sz;
		kc->kc_pa.pa_alloc = kmem_poolpage_alloc;
		kc->kc_pa.pa_free = kmem_poolpage_free;
		sprintf(kc->kc_name, "kmem-%zu", sz);
		kc->kc_cache = pool_cache_init(sz,
		    KMEM_QUANTUM_SIZE, 0, PR_NOALIGN | PR_NOTOUCH,
		    kc->kc_name, &kc->kc_pa, IPL_NONE,
		    NULL, NULL, NULL);
		KASSERT(kc->kc_cache != NULL);
	}
}

size_t
kmem_roundup_size(size_t size)
{

	return vmem_roundup_size(kmem_arena, size);
}

/* ---- uvm glue */

static vmem_addr_t
kmem_backend_alloc(vmem_t *dummy, vmem_size_t size, vmem_size_t *resultsize,
    vm_flag_t vmflags)
{
	uvm_flag_t uflags;
	vaddr_t va;

	KASSERT(dummy == NULL);
	KASSERT(size != 0);
	KASSERT((vmflags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	KASSERT((~vmflags & (VM_SLEEP|VM_NOSLEEP)) != 0);

	if ((vmflags & VM_NOSLEEP) != 0) {
		uflags = UVM_KMF_TRYLOCK | UVM_KMF_NOWAIT;
	} else {
		uflags = UVM_KMF_WAITVA;
	}
	*resultsize = size = round_page(size);
	va = uvm_km_alloc(kernel_map, size, 0,
	    uflags | UVM_KMF_WIRED | UVM_KMF_CANFAIL);
	if (va != 0) {
		kmem_poison_fill((void *)va, size);
	}
	return (vmem_addr_t)va;
}

static void
kmem_backend_free(vmem_t *dummy, vmem_addr_t addr, vmem_size_t size)
{

	KASSERT(dummy == NULL);
	KASSERT(addr != 0);
	KASSERT(size != 0);
	KASSERT(size == round_page(size));

	kmem_poison_check((void *)addr, size);
	uvm_km_free(kernel_map, (vaddr_t)addr, size, UVM_KMF_WIRED);
}

static int
kmem_kva_reclaim_callback(struct callback_entry *ce, void *obj, void *arg)
{
	vmem_t *vm = obj;

	vmem_reap(vm);
	return CALLBACK_CHAIN_CONTINUE;
}

/* ---- debug */

#if defined(KMEM_POISON)

#if defined(_LP64)
#define	PRIME	0x9e37fffffffc0001UL
#else /* defined(_LP64) */
#define	PRIME	0x9e3779b1
#endif /* defined(_LP64) */

static inline uint8_t
kmem_poison_pattern(const void *p)
{

	return (uint8_t)((((uintptr_t)p) * PRIME)
	    >> ((sizeof(uintptr_t) - sizeof(uint8_t))) * CHAR_BIT);
}

static void
kmem_poison_fill(void *p, size_t sz)
{
	uint8_t *cp;
	const uint8_t *ep;

	cp = p;
	ep = cp + sz;
	while (cp < ep) {
		*cp = kmem_poison_pattern(cp);
		cp++;
	}
}

static void
kmem_poison_check(void *p, size_t sz)
{
	uint8_t *cp;
	const uint8_t *ep;

	cp = p;
	ep = cp + sz;
	while (cp < ep) {
		const uint8_t expected = kmem_poison_pattern(cp);

		if (*cp != expected) {
			panic("%s: %p: 0x%02x != 0x%02x\n",
			    __func__, cp, *cp, expected);
		}
		cp++;
	}
}

#endif /* defined(KMEM_POISON) */

#if defined(KMEM_SIZE)
static void
kmem_size_set(void *p, size_t sz)
{

	memcpy(p, &sz, sizeof(sz));
}

static void
kmem_size_check(const void *p, size_t sz)
{
	size_t psz;

	memcpy(&psz, p, sizeof(psz));
	if (psz != sz) {
		panic("kmem_free(%p, %zu) != allocated size %zu",
		    (const uint8_t *)p + SIZE_SIZE, sz - SIZE_SIZE, psz);
	}
}
#endif	/* defined(KMEM_SIZE) */
