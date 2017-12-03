/*	$NetBSD: spinlock.h,v 1.3.4.3 2017/12/03 11:37:59 jdolecek Exp $	*/

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

#include <machine/limits.h>

#define	__acquires(lock)	/* XXX lockdep stuff */
#define	__releases(lock)	/* XXX lockdep stuff */

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
 * Stupid reader/writer spin locks.  No attempt to avoid writer
 * starvation.  Must allow recursive readers.  We use mutex and state
 * instead of compare-and-swap for expedience and LOCKDEBUG support.
 */

typedef struct linux_rwlock {
	kmutex_t	rw_lock;
	unsigned	rw_nreaders;
} rwlock_t;

static inline void
rwlock_init(rwlock_t *rw)
{

	mutex_init(&rw->rw_lock, MUTEX_DEFAULT, IPL_VM);
	rw->rw_nreaders = 0;
}

static inline void
rwlock_destroy(rwlock_t *rw)
{

	KASSERTMSG(rw->rw_nreaders == 0,
	    "rwlock still held by %u readers", rw->rw_nreaders);
	mutex_destroy(&rw->rw_lock);
}

static inline void
write_lock_irq(rwlock_t *rw)
{

	for (;;) {
		mutex_spin_enter(&rw->rw_lock);
		if (rw->rw_nreaders == 0)
			break;
		mutex_spin_exit(&rw->rw_lock);
	}
}

static inline void
write_unlock_irq(rwlock_t *rw)
{

	KASSERT(rw->rw_nreaders == 0);
	mutex_spin_exit(&rw->rw_lock);
}

static inline void
read_lock(rwlock_t *rw)
{

	mutex_spin_enter(&rw->rw_lock);
	KASSERT(rw->rw_nreaders < UINT_MAX);
	rw->rw_nreaders++;
	mutex_spin_exit(&rw->rw_lock);
}

static inline void
read_unlock(rwlock_t *rw)
{

	mutex_spin_enter(&rw->rw_lock);
	KASSERT(0 < rw->rw_nreaders);
	rw->rw_nreaders--;
	mutex_spin_exit(&rw->rw_lock);
}

#endif  /* _LINUX_SPINLOCK_H_ */
