/*	$NetBSD: subr_kmem.c,v 1.51.2.1 2014/08/10 06:55:58 tls Exp $	*/

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
 * Allocator of kernel wired memory. This allocator has some debug features
 * enabled with "option DIAGNOSTIC" and "option DEBUG".
 */

/*
 * KMEM_SIZE: detect alloc/free size mismatch bugs.
 *	Prefix each allocations with a fixed-sized, aligned header and record
 *	the exact user-requested allocation size in it. When freeing, compare
 *	it with kmem_free's "size" argument.
 *
 * KMEM_REDZONE: detect overrun bugs.
 *	Add a 2-byte pattern (allocate one more memory chunk if needed) at the
 *	end of each allocated buffer. Check this pattern on kmem_free.
 *
 * These options are enabled on DIAGNOSTIC.
 *
 *  |CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|
 *  +-----+-----+-----+-----+-----+-----+-----+-----+-----+---+-+--+--+
 *  |/////|     |     |     |     |     |     |     |     |   |*|**|UU|
 *  |/HSZ/|     |     |     |     |     |     |     |     |   |*|**|UU|
 *  |/////|     |     |     |     |     |     |     |     |   |*|**|UU|
 *  +-----+-----+-----+-----+-----+-----+-----+-----+-----+---+-+--+--+
 *  |Size |    Buffer usable by the caller (requested size)   |RedZ|Unused\
 */

/*
 * KMEM_POISON: detect modify-after-free bugs.
 *	Fill freed (in the sense of kmem_free) memory with a garbage pattern.
 *	Check the pattern on allocation.
 *
 * KMEM_GUARD
 *	A kernel with "option DEBUG" has "kmguard" debugging feature compiled
 *	in. See the comment in uvm/uvm_kmguard.c for what kind of bugs it tries
 *	to detect.  Even if compiled in, it's disabled by default because it's
 *	very expensive.  You can enable it on boot by:
 *		boot -d
 *		db> w kmem_guard_depth 0t30000
 *		db> c
 *
 *	The default value of kmem_guard_depth is 0, which means disabled.
 *	It can be changed by KMEM_GUARD_DEPTH kernel config option.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kmem.c,v 1.51.2.1 2014/08/10 06:55:58 tls Exp $");

#include <sys/param.h>
#include <sys/callback.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/debug.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm_kmguard.h>

#include <lib/libkern/libkern.h>

struct kmem_cache_info {
	size_t		kc_size;
	const char *	kc_name;
};

static const struct kmem_cache_info kmem_cache_sizes[] = {
	{  8, "kmem-8" },
	{ 16, "kmem-16" },
	{ 24, "kmem-24" },
	{ 32, "kmem-32" },
	{ 40, "kmem-40" },
	{ 48, "kmem-48" },
	{ 56, "kmem-56" },
	{ 64, "kmem-64" },
	{ 80, "kmem-80" },
	{ 96, "kmem-96" },
	{ 112, "kmem-112" },
	{ 128, "kmem-128" },
	{ 160, "kmem-160" },
	{ 192, "kmem-192" },
	{ 224, "kmem-224" },
	{ 256, "kmem-256" },
	{ 320, "kmem-320" },
	{ 384, "kmem-384" },
	{ 448, "kmem-448" },
	{ 512, "kmem-512" },
	{ 768, "kmem-768" },
	{ 1024, "kmem-1024" },
	{ 0, NULL }
};

static const struct kmem_cache_info kmem_cache_big_sizes[] = {
	{ 2048, "kmem-2048" },
	{ 4096, "kmem-4096" },
	{ 8192, "kmem-8192" },
	{ 16384, "kmem-16384" },
	{ 0, NULL }
};

/*
 * KMEM_ALIGN is the smallest guaranteed alignment and also the
 * smallest allocateable quantum.
 * Every cache size >= CACHE_LINE_SIZE gets CACHE_LINE_SIZE alignment.
 */
#define	KMEM_ALIGN		8
#define	KMEM_SHIFT		3
#define	KMEM_MAXSIZE		1024
#define	KMEM_CACHE_COUNT	(KMEM_MAXSIZE >> KMEM_SHIFT)

static pool_cache_t kmem_cache[KMEM_CACHE_COUNT] __cacheline_aligned;
static size_t kmem_cache_maxidx __read_mostly;

#define	KMEM_BIG_ALIGN		2048
#define	KMEM_BIG_SHIFT		11
#define	KMEM_BIG_MAXSIZE	16384
#define	KMEM_CACHE_BIG_COUNT	(KMEM_BIG_MAXSIZE >> KMEM_BIG_SHIFT)

static pool_cache_t kmem_cache_big[KMEM_CACHE_BIG_COUNT] __cacheline_aligned;
static size_t kmem_cache_big_maxidx __read_mostly;

#if defined(DIAGNOSTIC) && defined(_HARDKERNEL)
#define	KMEM_SIZE
#define	KMEM_REDZONE
#endif /* defined(DIAGNOSTIC) */

#if defined(DEBUG) && defined(_HARDKERNEL)
#define	KMEM_POISON
#define	KMEM_GUARD
#endif /* defined(DEBUG) */

#if defined(KMEM_POISON)
static int kmem_poison_ctor(void *, void *, int);
static void kmem_poison_fill(void *, size_t);
static void kmem_poison_check(void *, size_t);
#else /* defined(KMEM_POISON) */
#define	kmem_poison_fill(p, sz)		/* nothing */
#define	kmem_poison_check(p, sz)	/* nothing */
#endif /* defined(KMEM_POISON) */

#if defined(KMEM_REDZONE)
#define	REDZONE_SIZE	2
static void kmem_redzone_fill(void *, size_t);
static void kmem_redzone_check(void *, size_t);
#else /* defined(KMEM_REDZONE) */
#define	REDZONE_SIZE	0
#define	kmem_redzone_fill(p, sz)		/* nothing */
#define	kmem_redzone_check(p, sz)	/* nothing */
#endif /* defined(KMEM_REDZONE) */

#if defined(KMEM_SIZE)
struct kmem_header {
	size_t		size;
} __aligned(KMEM_ALIGN);
#define	SIZE_SIZE	sizeof(struct kmem_header)
static void kmem_size_set(void *, size_t);
static void kmem_size_check(void *, size_t);
#else
#define	SIZE_SIZE	0
#define	kmem_size_set(p, sz)	/* nothing */
#define	kmem_size_check(p, sz)	/* nothing */
#endif

#if defined(KMEM_GUARD)
#ifndef KMEM_GUARD_DEPTH
#define KMEM_GUARD_DEPTH 0
#endif
int kmem_guard_depth = KMEM_GUARD_DEPTH;
size_t kmem_guard_size;
static struct uvm_kmguard kmem_guard;
static void *kmem_freecheck;
#endif /* defined(KMEM_GUARD) */

CTASSERT(KM_SLEEP == PR_WAITOK);
CTASSERT(KM_NOSLEEP == PR_NOWAIT);

/*
 * kmem_intr_alloc: allocate wired memory.
 */

void *
kmem_intr_alloc(size_t requested_size, km_flag_t kmflags)
{
	size_t allocsz, index;
	size_t size;
	pool_cache_t pc;
	uint8_t *p;

	KASSERT(requested_size > 0);

#ifdef KMEM_GUARD
	if (requested_size <= kmem_guard_size) {
		return uvm_kmguard_alloc(&kmem_guard, requested_size,
		    (kmflags & KM_SLEEP) != 0);
	}
#endif
	size = kmem_roundup_size(requested_size);
	allocsz = size + SIZE_SIZE;

#ifdef KMEM_REDZONE
	if (size - requested_size < REDZONE_SIZE) {
		/* If there isn't enough space in the padding, allocate
		 * one more memory chunk for the red zone. */
		allocsz += kmem_roundup_size(REDZONE_SIZE);
	}
#endif

	if ((index = ((allocsz -1) >> KMEM_SHIFT))
	    < kmem_cache_maxidx) {
		pc = kmem_cache[index];
	} else if ((index = ((allocsz - 1) >> KMEM_BIG_SHIFT))
	    < kmem_cache_big_maxidx) {
		pc = kmem_cache_big[index];
	} else {
		int ret = uvm_km_kmem_alloc(kmem_va_arena,
		    (vsize_t)round_page(size),
		    ((kmflags & KM_SLEEP) ? VM_SLEEP : VM_NOSLEEP)
		     | VM_INSTANTFIT, (vmem_addr_t *)&p);
		if (ret) {
			return NULL;
		}
		FREECHECK_OUT(&kmem_freecheck, p);
		return p;
	}

	p = pool_cache_get(pc, kmflags);

	if (__predict_true(p != NULL)) {
		kmem_poison_check(p, allocsz);
		FREECHECK_OUT(&kmem_freecheck, p);
		kmem_size_set(p, requested_size);
		kmem_redzone_fill(p, requested_size + SIZE_SIZE);

		return p + SIZE_SIZE;
	}
	return p;
}

/*
 * kmem_intr_zalloc: allocate zeroed wired memory.
 */

void *
kmem_intr_zalloc(size_t size, km_flag_t kmflags)
{
	void *p;

	p = kmem_intr_alloc(size, kmflags);
	if (p != NULL) {
		memset(p, 0, size);
	}
	return p;
}

/*
 * kmem_intr_free: free wired memory allocated by kmem_alloc.
 */

void
kmem_intr_free(void *p, size_t requested_size)
{
	size_t allocsz, index;
	size_t size;
	pool_cache_t pc;

	KASSERT(p != NULL);
	KASSERT(requested_size > 0);

#ifdef KMEM_GUARD
	if (requested_size <= kmem_guard_size) {
		uvm_kmguard_free(&kmem_guard, requested_size, p);
		return;
	}
#endif

	size = kmem_roundup_size(requested_size);
	allocsz = size + SIZE_SIZE;

#ifdef KMEM_REDZONE
	if (size - requested_size < REDZONE_SIZE) {
		allocsz += kmem_roundup_size(REDZONE_SIZE);
	}
#endif

	if ((index = ((allocsz -1) >> KMEM_SHIFT))
	    < kmem_cache_maxidx) {
		pc = kmem_cache[index];
	} else if ((index = ((allocsz - 1) >> KMEM_BIG_SHIFT))
	    < kmem_cache_big_maxidx) {
		pc = kmem_cache_big[index];
	} else {
		FREECHECK_IN(&kmem_freecheck, p);
		uvm_km_kmem_free(kmem_va_arena, (vaddr_t)p,
		    round_page(size));
		return;
	}

	p = (uint8_t *)p - SIZE_SIZE;
	kmem_size_check(p, requested_size);
	kmem_redzone_check(p, requested_size + SIZE_SIZE);
	FREECHECK_IN(&kmem_freecheck, p);
	LOCKDEBUG_MEM_CHECK(p, size);
	kmem_poison_fill(p, allocsz);

	pool_cache_put(pc, p);
}

/* ---- kmem API */

/*
 * kmem_alloc: allocate wired memory.
 * => must not be called from interrupt context.
 */

void *
kmem_alloc(size_t size, km_flag_t kmflags)
{

	KASSERTMSG((!cpu_intr_p() && !cpu_softintr_p()),
	    "kmem(9) should not be used from the interrupt context");
	return kmem_intr_alloc(size, kmflags);
}

/*
 * kmem_zalloc: allocate zeroed wired memory.
 * => must not be called from interrupt context.
 */

void *
kmem_zalloc(size_t size, km_flag_t kmflags)
{

	KASSERTMSG((!cpu_intr_p() && !cpu_softintr_p()),
	    "kmem(9) should not be used from the interrupt context");
	return kmem_intr_zalloc(size, kmflags);
}

/*
 * kmem_free: free wired memory allocated by kmem_alloc.
 * => must not be called from interrupt context.
 */

void
kmem_free(void *p, size_t size)
{

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	kmem_intr_free(p, size);
}

static size_t
kmem_create_caches(const struct kmem_cache_info *array,
    pool_cache_t alloc_table[], size_t maxsize, int shift, int ipl)
{
	size_t maxidx = 0;
	size_t table_unit = (1 << shift);
	size_t size = table_unit;
	int i;

	for (i = 0; array[i].kc_size != 0 ; i++) {
		const char *name = array[i].kc_name;
		size_t cache_size = array[i].kc_size;
		struct pool_allocator *pa;
		int flags = PR_NOALIGN;
		pool_cache_t pc;
		size_t align;

		if ((cache_size & (CACHE_LINE_SIZE - 1)) == 0)
			align = CACHE_LINE_SIZE;
		else if ((cache_size & (PAGE_SIZE - 1)) == 0)
			align = PAGE_SIZE;
		else
			align = KMEM_ALIGN;

		if (cache_size < CACHE_LINE_SIZE)
			flags |= PR_NOTOUCH;

		/* check if we reached the requested size */
		if (cache_size > maxsize || cache_size > PAGE_SIZE) {
			break;
		}
		if ((cache_size >> shift) > maxidx) {
			maxidx = cache_size >> shift;
		}

		if ((cache_size >> shift) > maxidx) {
			maxidx = cache_size >> shift;
		}

		pa = &pool_allocator_kmem;
#if defined(KMEM_POISON)
		pc = pool_cache_init(cache_size, align, 0, flags,
		    name, pa, ipl, kmem_poison_ctor,
		    NULL, (void *)cache_size);
#else /* defined(KMEM_POISON) */
		pc = pool_cache_init(cache_size, align, 0, flags,
		    name, pa, ipl, NULL, NULL, NULL);
#endif /* defined(KMEM_POISON) */

		while (size <= cache_size) {
			alloc_table[(size - 1) >> shift] = pc;
			size += table_unit;
		}
	}
	return maxidx;
}

void
kmem_init(void)
{

#ifdef KMEM_GUARD
	uvm_kmguard_init(&kmem_guard, &kmem_guard_depth, &kmem_guard_size,
	    kmem_va_arena);
#endif
	kmem_cache_maxidx = kmem_create_caches(kmem_cache_sizes,
	    kmem_cache, KMEM_MAXSIZE, KMEM_SHIFT, IPL_VM);
	kmem_cache_big_maxidx = kmem_create_caches(kmem_cache_big_sizes,
	    kmem_cache_big, PAGE_SIZE, KMEM_BIG_SHIFT, IPL_VM);
}

size_t
kmem_roundup_size(size_t size)
{

	return (size + (KMEM_ALIGN - 1)) & ~(KMEM_ALIGN - 1);
}

/* ------------------ DEBUG / DIAGNOSTIC ------------------ */

#if defined(KMEM_POISON) || defined(KMEM_REDZONE)
#if defined(_LP64)
#define PRIME 0x9e37fffffffc0000UL
#else /* defined(_LP64) */
#define PRIME 0x9e3779b1
#endif /* defined(_LP64) */

static inline uint8_t
kmem_pattern_generate(const void *p)
{
	return (uint8_t)(((uintptr_t)p) * PRIME
	   >> ((sizeof(uintptr_t) - sizeof(uint8_t))) * CHAR_BIT);
}
#endif /* defined(KMEM_POISON) || defined(KMEM_REDZONE) */

#if defined(KMEM_POISON)
static int
kmem_poison_ctor(void *arg, void *obj, int flag)
{
	size_t sz = (size_t)arg;

	kmem_poison_fill(obj, sz);

	return 0;
}

static void
kmem_poison_fill(void *p, size_t sz)
{
	uint8_t *cp;
	const uint8_t *ep;

	cp = p;
	ep = cp + sz;
	while (cp < ep) {
		*cp = kmem_pattern_generate(cp);
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
		const uint8_t expected = kmem_pattern_generate(cp);

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
	struct kmem_header *hd;
	hd = (struct kmem_header *)p;
	hd->size = sz;
}

static void
kmem_size_check(void *p, size_t sz)
{
	struct kmem_header *hd;
	size_t hsz;

	hd = (struct kmem_header *)p;
	hsz = hd->size;

	if (hsz != sz) {
		panic("kmem_free(%p, %zu) != allocated size %zu",
		    (const uint8_t *)p + SIZE_SIZE, sz, hsz);
	}
}
#endif /* defined(KMEM_SIZE) */

#if defined(KMEM_REDZONE)
#define STATIC_BYTE	0xFE
CTASSERT(REDZONE_SIZE > 1);
static void
kmem_redzone_fill(void *p, size_t sz)
{
	uint8_t *cp, pat;
	const uint8_t *ep;

	cp = (uint8_t *)p + sz;
	ep = cp + REDZONE_SIZE;

	/*
	 * We really don't want the first byte of the red zone to be '\0';
	 * an off-by-one in a string may not be properly detected.
	 */
	pat = kmem_pattern_generate(cp);
	*cp = (pat == '\0') ? STATIC_BYTE: pat;
	cp++;

	while (cp < ep) {
		*cp = kmem_pattern_generate(cp);
		cp++;
	}
}

static void
kmem_redzone_check(void *p, size_t sz)
{
	uint8_t *cp, pat, expected;
	const uint8_t *ep;

	cp = (uint8_t *)p + sz;
	ep = cp + REDZONE_SIZE;

	pat = kmem_pattern_generate(cp);
	expected = (pat == '\0') ? STATIC_BYTE: pat;
	if (expected != *cp) {
		panic("%s: %p: 0x%02x != 0x%02x\n",
		   __func__, cp, *cp, expected);
	}
	cp++;

	while (cp < ep) {
		expected = kmem_pattern_generate(cp);
		if (*cp != expected) {
			panic("%s: %p: 0x%02x != 0x%02x\n",
			   __func__, cp, *cp, expected);
		}
		cp++;
	}
}
#endif /* defined(KMEM_REDZONE) */


/*
 * Used to dynamically allocate string with kmem accordingly to format.
 */
char *
kmem_asprintf(const char *fmt, ...)
{
	int size __diagused, len;
	va_list va;
	char *str;

	va_start(va, fmt);
	len = vsnprintf(NULL, 0, fmt, va);
	va_end(va);

	str = kmem_alloc(len + 1, KM_SLEEP);

	va_start(va, fmt);
	size = vsnprintf(str, len + 1, fmt, va);
	va_end(va);

	KASSERT(size == len);

	return str;
}
