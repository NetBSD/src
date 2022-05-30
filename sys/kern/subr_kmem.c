/*	$NetBSD: subr_kmem.c,v 1.87 2022/05/30 23:36:26 mrg Exp $	*/

/*
 * Copyright (c) 2009-2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Maxime Villard.
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

/*
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
 *	Append to each allocation a fixed-sized footer and record the exact
 *	user-requested allocation size in it.  When freeing, compare it with
 *	kmem_free's "size" argument.
 *
 * This option is enabled on DIAGNOSTIC.
 *
 *  |CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK|CHUNK| |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-+
 *  |     |     |     |     |     |     |     |     |/////|U|
 *  |     |     |     |     |     |     |     |     |/HSZ/|U|
 *  |     |     |     |     |     |     |     |     |/////|U|
 *  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-+
 *  | Buffer usable by the caller (requested size)  |Size |Unused
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kmem.c,v 1.87 2022/05/30 23:36:26 mrg Exp $");

#ifdef _KERNEL_OPT
#include "opt_kmem.h"
#endif

#include <sys/param.h>
#include <sys/callback.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/debug.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>
#include <sys/asan.h>
#include <sys/msan.h>
#include <sys/sdt.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <lib/libkern/libkern.h>

struct kmem_cache_info {
	size_t		kc_size;
	const char *	kc_name;
#ifdef KDTRACE_HOOKS
	const id_t	*kc_alloc_probe_id;
	const id_t	*kc_free_probe_id;
#endif
};

#define	KMEM_CACHE_SIZES(F)						      \
	F(8, kmem-00008, kmem__00008)					      \
	F(16, kmem-00016, kmem__00016)					      \
	F(24, kmem-00024, kmem__00024)					      \
	F(32, kmem-00032, kmem__00032)					      \
	F(40, kmem-00040, kmem__00040)					      \
	F(48, kmem-00048, kmem__00048)					      \
	F(56, kmem-00056, kmem__00056)					      \
	F(64, kmem-00064, kmem__00064)					      \
	F(80, kmem-00080, kmem__00080)					      \
	F(96, kmem-00096, kmem__00096)					      \
	F(112, kmem-00112, kmem__00112)					      \
	F(128, kmem-00128, kmem__00128)					      \
	F(160, kmem-00160, kmem__00160)					      \
	F(192, kmem-00192, kmem__00192)					      \
	F(224, kmem-00224, kmem__00224)					      \
	F(256, kmem-00256, kmem__00256)					      \
	F(320, kmem-00320, kmem__00320)					      \
	F(384, kmem-00384, kmem__00384)					      \
	F(448, kmem-00448, kmem__00448)					      \
	F(512, kmem-00512, kmem__00512)					      \
	F(768, kmem-00768, kmem__00768)					      \
	F(1024, kmem-01024, kmem__01024)				      \
	/* end of KMEM_CACHE_SIZES */

#define	KMEM_CACHE_BIG_SIZES(F)						      \
	F(2048, kmem-02048, kmem__02048)				      \
	F(4096, kmem-04096, kmem__04096)				      \
	F(8192, kmem-08192, kmem__08192)				      \
	F(16384, kmem-16384, kmem__16384)				      \
	/* end of KMEM_CACHE_BIG_SIZES */

/* sdt:kmem:alloc:kmem-* probes */
#define	F(SZ, NAME, PROBENAME)						      \
	SDT_PROBE_DEFINE4(sdt, kmem, alloc, PROBENAME,			      \
	    "void *"/*ptr*/,						      \
	    "size_t"/*requested_size*/,					      \
	    "size_t"/*allocated_size*/,					      \
	    "km_flag_t"/*kmflags*/);
KMEM_CACHE_SIZES(F);
KMEM_CACHE_BIG_SIZES(F);
#undef	F

/* sdt:kmem:free:kmem-* probes */
#define	F(SZ, NAME, PROBENAME)						      \
	SDT_PROBE_DEFINE3(sdt, kmem, free, PROBENAME,			      \
	    "void *"/*ptr*/,						      \
	    "size_t"/*requested_size*/,					      \
	    "size_t"/*allocated_size*/);
KMEM_CACHE_SIZES(F);
KMEM_CACHE_BIG_SIZES(F);
#undef	F

/* sdt:kmem:alloc:large, sdt:kmem:free:large probes */
SDT_PROBE_DEFINE4(sdt, kmem, alloc, large,
    "void *"/*ptr*/,
    "size_t"/*requested_size*/,
    "size_t"/*allocated_size*/,
    "km_flag_t"/*kmflags*/);
SDT_PROBE_DEFINE3(sdt, kmem, free, large,
    "void *"/*ptr*/,
    "size_t"/*requested_size*/,
    "size_t"/*allocated_size*/);

#ifdef KDTRACE_HOOKS
#define	F(SZ, NAME, PROBENAME)						      \
	{ SZ, #NAME,							      \
	  &sdt_sdt_kmem_alloc_##PROBENAME->id,				      \
	  &sdt_sdt_kmem_free_##PROBENAME->id },
#else
#define	F(SZ, NAME, PROBENAME)	{ SZ, #NAME },
#endif

static const struct kmem_cache_info kmem_cache_sizes[] = {
	KMEM_CACHE_SIZES(F)
	{ 0 }
};

static const struct kmem_cache_info kmem_cache_big_sizes[] = {
	KMEM_CACHE_BIG_SIZES(F)
	{ 0 }
};

#undef	F

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
#endif

#if defined(DEBUG) && defined(_HARDKERNEL)
static void *kmem_freecheck;
#endif

#if defined(KMEM_SIZE)
#define	SIZE_SIZE	sizeof(size_t)
static void kmem_size_set(void *, size_t);
static void kmem_size_check(void *, size_t);
#else
#define	SIZE_SIZE	0
#define	kmem_size_set(p, sz)	/* nothing */
#define	kmem_size_check(p, sz)	/* nothing */
#endif

#ifndef KDTRACE_HOOKS

static const id_t **const kmem_cache_alloc_probe_id = NULL;
static const id_t **const kmem_cache_big_alloc_probe_id = NULL;
static const id_t **const kmem_cache_free_probe_id = NULL;
static const id_t **const kmem_cache_big_free_probe_id = NULL;

#define	KMEM_CACHE_PROBE(ARRAY, INDEX, PTR, REQSIZE, ALLOCSIZE, FLAGS)	      \
	__nothing

#else

static const id_t *kmem_cache_alloc_probe_id[KMEM_CACHE_COUNT];
static const id_t *kmem_cache_big_alloc_probe_id[KMEM_CACHE_COUNT];
static const id_t *kmem_cache_free_probe_id[KMEM_CACHE_COUNT];
static const id_t *kmem_cache_big_free_probe_id[KMEM_CACHE_COUNT];

#define	KMEM_CACHE_PROBE(ARRAY, INDEX, PTR, REQSIZE, ALLOCSIZE, FLAGS) do     \
{									      \
	id_t id;							      \
									      \
	KDASSERT((INDEX) < __arraycount(ARRAY));			      \
	if (__predict_false((id = *(ARRAY)[INDEX]) != 0)) {		      \
		(*sdt_probe_func)(id,					      \
		    (uintptr_t)(PTR),					      \
		    (uintptr_t)(REQSIZE),				      \
		    (uintptr_t)(ALLOCSIZE),				      \
		    (uintptr_t)(FLAGS),					      \
		    (uintptr_t)0);					      \
	}								      \
} while (0)

#endif	/* KDTRACE_HOOKS */

#define	KMEM_CACHE_ALLOC_PROBE(I, P, RS, AS, F)				      \
	KMEM_CACHE_PROBE(kmem_cache_alloc_probe_id, I, P, RS, AS, F)
#define	KMEM_CACHE_BIG_ALLOC_PROBE(I, P, RS, AS, F)			      \
	KMEM_CACHE_PROBE(kmem_cache_big_alloc_probe_id, I, P, RS, AS, F)
#define	KMEM_CACHE_FREE_PROBE(I, P, RS, AS)				      \
	KMEM_CACHE_PROBE(kmem_cache_free_probe_id, I, P, RS, AS, 0)
#define	KMEM_CACHE_BIG_FREE_PROBE(I, P, RS, AS)				      \
	KMEM_CACHE_PROBE(kmem_cache_big_free_probe_id, I, P, RS, AS, 0)

CTASSERT(KM_SLEEP == PR_WAITOK);
CTASSERT(KM_NOSLEEP == PR_NOWAIT);

/*
 * kmem_intr_alloc: allocate wired memory.
 */
void *
kmem_intr_alloc(size_t requested_size, km_flag_t kmflags)
{
#ifdef KASAN
	const size_t origsize = requested_size;
#endif
	size_t allocsz, index;
	size_t size;
	pool_cache_t pc;
	uint8_t *p;

	KASSERT(requested_size > 0);

	KASSERT((kmflags & KM_SLEEP) || (kmflags & KM_NOSLEEP));
	KASSERT(!(kmflags & KM_SLEEP) || !(kmflags & KM_NOSLEEP));

	kasan_add_redzone(&requested_size);
	size = kmem_roundup_size(requested_size);
	allocsz = size + SIZE_SIZE;

	if ((index = ((allocsz - 1) >> KMEM_SHIFT))
	    < kmem_cache_maxidx) {
		pc = kmem_cache[index];
		p = pool_cache_get(pc, kmflags);
		KMEM_CACHE_ALLOC_PROBE(index,
		    p, requested_size, allocsz, kmflags);
	} else if ((index = ((allocsz - 1) >> KMEM_BIG_SHIFT))
	    < kmem_cache_big_maxidx) {
		pc = kmem_cache_big[index];
		p = pool_cache_get(pc, kmflags);
		KMEM_CACHE_BIG_ALLOC_PROBE(index,
		    p, requested_size, allocsz, kmflags);
	} else {
		int ret = uvm_km_kmem_alloc(kmem_va_arena,
		    (vsize_t)round_page(size),
		    ((kmflags & KM_SLEEP) ? VM_SLEEP : VM_NOSLEEP)
		     | VM_INSTANTFIT, (vmem_addr_t *)&p);
		SDT_PROBE4(sdt, kmem, alloc, large,
		    ret ? NULL : p, requested_size, round_page(size), kmflags);
		if (ret) {
			return NULL;
		}
		FREECHECK_OUT(&kmem_freecheck, p);
		return p;
	}

	if (__predict_true(p != NULL)) {
		FREECHECK_OUT(&kmem_freecheck, p);
		kmem_size_set(p, requested_size);
		kasan_mark(p, origsize, size, KASAN_KMEM_REDZONE);
		return p;
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
	KASSERTMSG(requested_size > 0, "kmem_intr_free(%p, 0)", p);

	kasan_add_redzone(&requested_size);
	size = kmem_roundup_size(requested_size);
	allocsz = size + SIZE_SIZE;

	if ((index = ((allocsz - 1) >> KMEM_SHIFT))
	    < kmem_cache_maxidx) {
		KMEM_CACHE_FREE_PROBE(index, p, requested_size, allocsz);
		pc = kmem_cache[index];
	} else if ((index = ((allocsz - 1) >> KMEM_BIG_SHIFT))
	    < kmem_cache_big_maxidx) {
		KMEM_CACHE_BIG_FREE_PROBE(index, p, requested_size, allocsz);
		pc = kmem_cache_big[index];
	} else {
		FREECHECK_IN(&kmem_freecheck, p);
		SDT_PROBE3(sdt, kmem, free, large,
		    p, requested_size, round_page(size));
		uvm_km_kmem_free(kmem_va_arena, (vaddr_t)p,
		    round_page(size));
		return;
	}

	kasan_mark(p, size, size, 0);

	kmem_size_check(p, requested_size);
	FREECHECK_IN(&kmem_freecheck, p);
	LOCKDEBUG_MEM_CHECK(p, size);

	pool_cache_put(pc, p);
}

/* -------------------------------- Kmem API -------------------------------- */

/*
 * kmem_alloc: allocate wired memory.
 * => must not be called from interrupt context.
 */
void *
kmem_alloc(size_t size, km_flag_t kmflags)
{
	void *v;

	KASSERTMSG((!cpu_intr_p() && !cpu_softintr_p()),
	    "kmem(9) should not be used from the interrupt context");
	v = kmem_intr_alloc(size, kmflags);
	if (__predict_true(v != NULL)) {
		kmsan_mark(v, size, KMSAN_STATE_UNINIT);
		kmsan_orig(v, size, KMSAN_TYPE_KMEM, __RET_ADDR);
	}
	KASSERT(v || (kmflags & KM_NOSLEEP) != 0);
	return v;
}

/*
 * kmem_zalloc: allocate zeroed wired memory.
 * => must not be called from interrupt context.
 */
void *
kmem_zalloc(size_t size, km_flag_t kmflags)
{
	void *v;

	KASSERTMSG((!cpu_intr_p() && !cpu_softintr_p()),
	    "kmem(9) should not be used from the interrupt context");
	v = kmem_intr_zalloc(size, kmflags);
	KASSERT(v || (kmflags & KM_NOSLEEP) != 0);
	return v;
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
	kmsan_mark(p, size, KMSAN_STATE_INITED);
}

static size_t
kmem_create_caches(const struct kmem_cache_info *array,
    const id_t *alloc_probe_table[], const id_t *free_probe_table[],
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
		int flags = 0;
		pool_cache_t pc;
		size_t align;

		/* check if we reached the requested size */
		if (cache_size > maxsize || cache_size > PAGE_SIZE) {
			break;
		}

		/*
		 * Exclude caches with size not a factor or multiple of the
		 * coherency unit.
		 */
		if (cache_size < COHERENCY_UNIT) {
			if (COHERENCY_UNIT % cache_size > 0) {
			    	continue;
			}
			flags |= PR_NOTOUCH;
			align = KMEM_ALIGN;
		} else if ((cache_size & (PAGE_SIZE - 1)) == 0) {
			align = PAGE_SIZE;
		} else {
			if ((cache_size % COHERENCY_UNIT) > 0) {
				continue;
			}
			align = COHERENCY_UNIT;
		}

		if ((cache_size >> shift) > maxidx) {
			maxidx = cache_size >> shift;
		}

		pa = &pool_allocator_kmem;
		pc = pool_cache_init(cache_size, align, 0, flags,
		    name, pa, ipl, NULL, NULL, NULL);

		while (size <= cache_size) {
			alloc_table[(size - 1) >> shift] = pc;
#ifdef KDTRACE_HOOKS
			if (alloc_probe_table) {
				alloc_probe_table[(size - 1) >> shift] =
				    array[i].kc_alloc_probe_id;
			}
			if (free_probe_table) {
				free_probe_table[(size - 1) >> shift] =
				    array[i].kc_free_probe_id;
			}
#endif
			size += table_unit;
		}
	}
	return maxidx;
}

void
kmem_init(void)
{
	kmem_cache_maxidx = kmem_create_caches(kmem_cache_sizes,
	    kmem_cache_alloc_probe_id, kmem_cache_free_probe_id,
	    kmem_cache, KMEM_MAXSIZE, KMEM_SHIFT, IPL_VM);
	kmem_cache_big_maxidx = kmem_create_caches(kmem_cache_big_sizes,
	    kmem_cache_big_alloc_probe_id, kmem_cache_big_free_probe_id,
	    kmem_cache_big, PAGE_SIZE, KMEM_BIG_SHIFT, IPL_VM);
}

size_t
kmem_roundup_size(size_t size)
{
	return (size + (KMEM_ALIGN - 1)) & ~(KMEM_ALIGN - 1);
}

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

char *
kmem_strdupsize(const char *str, size_t *lenp, km_flag_t flags)
{
	size_t len = strlen(str) + 1;
	char *ptr = kmem_alloc(len, flags);
	if (ptr == NULL)
		return NULL;

	if (lenp)
		*lenp = len;
	memcpy(ptr, str, len);
	return ptr;
}

char *
kmem_strndup(const char *str, size_t maxlen, km_flag_t flags)
{
	KASSERT(str != NULL);
	KASSERT(maxlen != 0);

	size_t len = strnlen(str, maxlen);
	char *ptr = kmem_alloc(len + 1, flags);
	if (ptr == NULL)
		return NULL;

	memcpy(ptr, str, len);
	ptr[len] = '\0';

	return ptr;
}

void
kmem_strfree(char *str)
{
	if (str == NULL)
		return;

	kmem_free(str, strlen(str) + 1);
}

/*
 * Utility routine to maybe-allocate a temporary buffer if the size
 * is larger than we're willing to put on the stack.
 */
void *
kmem_tmpbuf_alloc(size_t size, void *stackbuf, size_t stackbufsize,
    km_flag_t flags)
{
	if (size <= stackbufsize) {
		return stackbuf;
	}

	return kmem_alloc(size, flags);
}

void
kmem_tmpbuf_free(void *buf, size_t size, void *stackbuf)
{
	if (buf != stackbuf) {
		kmem_free(buf, size);
	}
}

/* --------------------------- DEBUG / DIAGNOSTIC --------------------------- */

#if defined(KMEM_SIZE)
static void
kmem_size_set(void *p, size_t sz)
{
	memcpy((char *)p + sz, &sz, sizeof(size_t));
}

static void
kmem_size_check(void *p, size_t sz)
{
	size_t hsz;

	memcpy(&hsz, (char *)p + sz, sizeof(size_t));

	if (hsz != sz) {
		panic("kmem_free(%p, %zu) != allocated size %zu; overwrote?",
		    p, sz, hsz);
	}

	memset((char *)p + sz, 0xff, sizeof(size_t));
}
#endif /* defined(KMEM_SIZE) */
