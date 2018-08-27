/*	$NetBSD: linux_atomic64.c,v 1.2 2018/08/27 15:10:53 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_atomic64.c,v 1.2 2018/08/27 15:10:53 riastradh Exp $");

#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/lock.h>

#include <linux/atomic.h>

#ifdef __HAVE_ATOMIC64_OPS

int
linux_atomic64_init(void)
{
	return 0;
}

void
linux_atomic64_fini(void)
{
}

#else

static struct {
	kmutex_t	lock;
	uint32_t	gen;	/* for unlocked read */
	char		pad[CACHE_LINE_SIZE -
			    sizeof(kmutex_t) - sizeof(uint32_t)];
} atomic64_tab[PAGE_SIZE/CACHE_LINE_SIZE] __cacheline_aligned;
CTASSERT(sizeof(atomic64_tab) == PAGE_SIZE);
CTASSERT(sizeof(atomic64_tab[0]) == CACHE_LINE_SIZE);

int
linux_atomic64_init(void)
{
	size_t i;

	for (i = 0; i < __arraycount(atomic64_tab); i++) {
		mutex_init(&atomic64_tab[i].lock, MUTEX_DEFAULT, IPL_HIGH);
		atomic64_tab[i].gen = 0;
	}

	return 0;
}

void
linux_atomic64_fini(void)
{
	size_t i;

	for (i = 0; i < __arraycount(atomic64_tab); i++) {
		KASSERT((atomic64_tab[i].gen & 1) == 0);
		mutex_destroy(&atomic64_tab[i].lock);
	}
}

static inline size_t
atomic64_hash(const struct atomic64 *a)
{

	return ((uintptr_t)a >> ilog2(CACHE_LINE_SIZE)) %
	    __arraycount(atomic64_tab);
}

static void
atomic64_lock(struct atomic64 *a)
{
	size_t i = atomic64_hash(a);

	mutex_spin_enter(&atomic64_tab[i].lock);
	KASSERT((atomic64_tab[i].gen & 1) == 0);
	atomic64_tab[i].gen |= 1;
	membar_producer();
}

static void
atomic64_unlock(struct atomic64 *a)
{
	size_t i = atomic64_hash(a);

	KASSERT(mutex_owned(&atomic64_tab[i].lock));
	KASSERT((atomic64_tab[i].gen & 1) == 1);

	membar_producer();
	atomic64_tab[i].gen |= 1; /* paranoia */
	atomic64_tab[i].gen++;
	mutex_spin_exit(&atomic64_tab[i].lock);
}

uint64_t
atomic64_read(const struct atomic64 *a)
{
	size_t i = atomic64_hash(a);
	uint32_t gen;
	uint64_t value;

	do {
		while (__predict_false((gen = atomic64_tab[i].gen) & 1))
			SPINLOCK_BACKOFF_HOOK;
		membar_consumer();
		value = a->a_v;
		membar_consumer();
	} while (__predict_false(atomic64_tab[i].gen != gen));

	return value;
}

void
atomic64_set(struct atomic64 *a, uint64_t value)
{

	atomic64_lock(a);
	a->a_v = value;
	atomic64_unlock(a);
}

void
atomic64_add(int64_t delta, struct atomic64 *a)
{

	atomic64_lock(a);
	a->a_v += delta;
	atomic64_unlock(a);
}

void
atomic64_sub(int64_t delta, struct atomic64 *a)
{

	atomic64_lock(a);
	a->a_v -= delta;
	atomic64_unlock(a);
}

uint64_t
atomic64_xchg(struct atomic64 *a, uint64_t new)
{
	uint64_t old;

	atomic64_lock(a);
	old = a->a_v;
	a->a_v = new;
	atomic64_unlock(a);

	return old;
}

uint64_t
atomic64_cmpxchg(struct atomic64 *a, uint64_t expect, uint64_t new)
{
	uint64_t old;

	atomic64_lock(a);
	old = a->a_v;
	if (old == expect)
		a->a_v = new;
	atomic64_unlock(a);

	return old;
}

#endif
