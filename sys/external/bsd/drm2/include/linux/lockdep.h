/*	$NetBSD: lockdep.h,v 1.8 2021/12/19 11:38:37 riastradh Exp $	*/

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

#ifndef _LINUX_LOCKDEP_H_
#define _LINUX_LOCKDEP_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <lib/libkern/libkern.h>

struct lock_class_key;
struct mutex;
struct spinlock;

#define	__acquires(lock)	/* XXX lockdep annotation */
#define	__releases(lock)	/* XXX lockdep annotation */

#define	mutex_acquire(m, x, y, ip)	__nothing
#define	mutex_release(m, ip)		__nothing

#define	__lockdep_used			__unused
#define	lock_acquire_shared_recursive(l, s, t, n, i)	__nothing
#define	lock_release(l, n)		__nothing
#ifdef notyet
#define	lockdep_assert_held(m)		KDASSERT(lockdep_is_held(m))
#define	lockdep_assert_held_once(m)	KDASSERT(lockdep_is_held(m))
#define	lockdep_is_held(m)		mutex_owned(__lockdep_kmutex(m))
#define	might_lock(m)							      \
	KDASSERT(mutex_ownable(__lockdep_kmutex(m)))
#else
#define	lockdep_assert_held(m)		do {} while (0)
#define	lockdep_assert_held_once(m)	do {} while (0)
#define	lockdep_is_held(m)		1
#define	might_lock(m)			do {} while (0)
#define	might_lock_nested(m,n)		do {} while (0)
#endif

#define	__lockdep_kmutex(m)						      \
	(__builtin_types_compatible_p(typeof(*(m)), struct mutex) ?	      \
		&((const struct mutex *)(m))->mtx_lock			      \
	    : __builtin_types_compatible_p(typeof(*(m)), struct spinlock) ?   \
		&((const struct spinlock *)(m))->sl_lock		      \
	    : NULL)

#define	SINGLE_DEPTH_NESTING	0

struct pin_cookie {
	int	dummy;
};

static inline struct pin_cookie
lockdep_pin_lock(struct mutex *m)
{
	return (struct pin_cookie) { (int)(intptr_t)m };
}

static inline void
lockdep_unpin_lock(struct mutex *m, struct pin_cookie cookie)
{
	KASSERT(cookie.dummy == (int)(intptr_t)m);
}

static inline void
lockdep_set_class(void *m, struct lock_class_key *ck)
{
}

static inline void
lockdep_set_subclass(void *m, unsigned subclass)
{
}

#endif	/* _LINUX_LOCKDEP_H_ */
