/*	$NetBSD: subr_kmem.c,v 1.83 2022/03/12 16:54:17 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_kmem.c,v 1.83 2022/03/12 16:54:17 riastradh Exp $");

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

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <lib/libkern/libkern.h>

struct kmem_cache_info {
	size_t		kc_size;
	const char *	kc_name;
};

static const struct kmem_cache_info kmem_cache_sizes[] = {
	{  8, "kmem-00008" },
	{ 16, "kmem-00016" },
	{ 24, "kmem-00024" },
	{ 32, "kmem-00032" },
	{ 40, "kmem-00040" },
	{ 48, "kmem-00048" },
	{ 56, "kmem-00056" },
	{ 64, "kmem-00064" },
	{ 80, "kmem-00080" },
	{ 96, "kmem-00096" },
	{ 112, "kmem-00112" },
	{ 128, "kmem-00128" },
	{ 160, "kmem-00160" },
	{ 192, "kmem-00192" },
	{ 224, "kmem-00224" },
	{ 256, "kmem-00256" },
	{ 320, "kmem-00320" },
	{ 384, "kmem-00384" },
	{ 448, "kmem-00448" },
	{ 512, "kmem-00512" },
	{ 768, "kmem-00768" },
	{ 1024, "kmem-01024" },
	{ 0, NULL }
};

static const struct kmem_cache_info kmem_cache_big_sizes[] = {
	{ 2048, "kmem-02048" },
	{ 4096, "kmem-04096" },
	{ 8192, "kmem-08192" },
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
	KASSERT(requested_size > 0);

	kasan_add_redzone(&requested_size);
	size = kmem_roundup_size(requested_size);
	allocsz = size + SIZE_SIZE;

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
			size += table_unit;
		}
	}
	return maxidx;
}

void
kmem_init(void)
{
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
