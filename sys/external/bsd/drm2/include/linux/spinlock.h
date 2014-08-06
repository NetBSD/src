/*	$NetBSD: spinlock.h,v 1.3 2014/08/06 13:53:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_SPINLOCK_H_
#define _LINUX_SPINLOCK_H_

#include <sys/cdefs.h>
#include <sys/mutex.h>

typedef struct spinlock {
	kmutex_t sl_lock;
} spinlock_t;

static inline int
spin_is_locked(spinlock_t *spinlock)
{
	return mutex_owned(&spinlock->sl_lock);
}

static inline void
spin_lock(spinlock_t *spinlock)
{
	mutex_enter(&spinlock->sl_lock);
}

static inline void
spin_unlock(spinlock_t *spinlock)
{
	mutex_exit(&spinlock->sl_lock);
}

static inline void
spin_lock_irq(spinlock_t *spinlock)
{
	spin_lock(spinlock);
}

static inline void
spin_unlock_irq(spinlock_t *spinlock)
{
	spin_unlock(spinlock);
}

/* Must be a macro because the second argument is to be assigned.  */
#define	spin_lock_irqsave(SPINLOCK, FLAGS)				\
	do {								\
		(FLAGS) = 0;						\
		mutex_enter(&((spinlock_t *)(SPINLOCK))->sl_lock);	\
	} while (0)

static inline void
spin_unlock_irqrestore(spinlock_t *spinlock, unsigned long __unused flags)
{
	mutex_exit(&spinlock->sl_lock);
}

static inline void
spin_lock_init(spinlock_t *spinlock)
{
	/* XXX What's the right IPL?  IPL_DRM...?  */
	mutex_init(&spinlock->sl_lock, MUTEX_DEFAULT, IPL_VM);
}

/*
 * XXX Linux doesn't ever destroy spin locks, it seems.  We'll have to
 * kludge it up.
 */

static inline void
spin_lock_destroy(spinlock_t *spinlock)
{
	mutex_destroy(&spinlock->sl_lock);
}

/* This is a macro to make the panic message clearer.  */
#define	assert_spin_locked(spinlock)	\
	KASSERT(mutex_owned(&(spinlock)->sl_lock))

/*
 * Linux rwlocks are reader/writer spin locks.  We implement them as
 * normal spin locks without reader/writer semantics for expedience.
 * If that turns out to not work, adapting to reader/writer semantics
 * shouldn't be too hard.
 */

#define	rwlock_t		spinlock_t
#define	rwlock_init		spin_lock_init
#define	rwlock_destroy		spin_lock_destroy
#define	write_lock_irq		spin_lock_irq
#define	write_unlock_irq	spin_unlock_irq
#define	read_lock		spin_lock
#define	read_unlock		spin_unlock

#endif  /* _LINUX_SPINLOCK_H_ */
