/*	$NetBSD: refcount.h,v 1.2 2021/12/19 10:48:37 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_REFCOUNT_H_
#define _LINUX_REFCOUNT_H_

#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

typedef struct refcount refcount_t;

struct refcount {
	atomic_t	rc_count;
};

static inline void
refcount_set(struct refcount *rc, int n)
{
	atomic_set(&rc->rc_count, n);
}

static inline void
refcount_inc(struct refcount *rc)
{
	atomic_inc(&rc->rc_count);
}

static inline int __must_check
refcount_inc_not_zero(struct refcount *rc)
{
	unsigned old, new;

	do {
		old = atomic_read(&rc->rc_count);
		if (old == 0)
			break;
		new = old + 1;
	} while (atomic_cmpxchg(&rc->rc_count, old, new) != old);

	return old;
}

static inline bool __must_check
refcount_dec_and_test(struct refcount *rc)
{
	unsigned old, new;

	do {
		old = atomic_read(&rc->rc_count);
		KASSERT(old);
		new = old - 1;
	} while (atomic_cmpxchg(&rc->rc_count, old, new) != old);

	return old == 1;
}

static inline bool __must_check
refcount_dec_and_lock_irqsave(struct refcount *rc, struct spinlock *lock,
    unsigned long *flagsp)
{
	unsigned old, new;

	do {
		old = atomic_read(&rc->rc_count);
		KASSERT(old);
		if (old == 1) {
			spin_lock_irqsave(lock, *flagsp);
			if (atomic_dec_return(&rc->rc_count) == 0)
				return true;
			spin_unlock_irqrestore(lock, *flagsp);
			return false;
		}
		new = old - 1;
	} while (atomic_cmpxchg(&rc->rc_count, old, new) != old);

	KASSERT(old != 1);
	KASSERT(new != 0);
	return false;
}

static inline bool __must_check
refcount_dec_and_mutex_lock(struct refcount *rc, struct mutex *lock)
{
	unsigned old, new;

	do {
		old = atomic_read(&rc->rc_count);
		KASSERT(old);
		if (old == 1) {
			mutex_lock(lock);
			if (atomic_dec_return(&rc->rc_count) == 0)
				return true;
			mutex_unlock(lock);
			return false;
		}
		new = old - 1;
	} while (atomic_cmpxchg(&rc->rc_count, old, new) != old);

	KASSERT(old != 1);
	KASSERT(new != 0);
	return false;
}

#endif  /* _LINUX_REFCOUNT_H_ */
