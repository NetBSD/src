/*	$NetBSD: mutex.h,v 1.17 2021/12/19 11:33:31 riastradh Exp $	*/

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

#ifndef _LINUX_MUTEX_H_
#define _LINUX_MUTEX_H_

#include <sys/mutex.h>

#include <lib/libkern/libkern.h> /* KASSERT */

#include <asm/processor.h>

#include <linux/list.h>
#include <linux/spinlock.h>

struct mutex {
	kmutex_t mtx_lock;
};

struct lock_class_key {
};

/* Name collision.  Pooh.  */
static inline void
linux_mutex_init(struct mutex *mutex)
{
	mutex_init(&mutex->mtx_lock, MUTEX_DEFAULT, IPL_NONE);
}

/* Lockdep stuff.  */
static inline void
__mutex_init(struct mutex *mutex, const char *name __unused,
    struct lock_class_key *key __unused)
{
	linux_mutex_init(mutex);
}

/* Another name collision.  */
static inline void
linux_mutex_destroy(struct mutex *mutex)
{
	mutex_destroy(&mutex->mtx_lock);
}

static inline void
mutex_lock(struct mutex *mutex)
{
	mutex_enter(&mutex->mtx_lock);
}

static inline int
mutex_lock_interruptible(struct mutex *mutex)
{
	mutex_enter(&mutex->mtx_lock); /* XXX */
	return 0;
}

static inline int
mutex_trylock(struct mutex *mutex)
{
	return mutex_tryenter(&mutex->mtx_lock);
}

static inline void
mutex_unlock(struct mutex *mutex)
{
	mutex_exit(&mutex->mtx_lock);
}

static inline bool
mutex_is_locked(struct mutex *mutex)
{
	return mutex_owned(&mutex->mtx_lock);
}

static inline void
mutex_lock_nest_lock(struct mutex *mutex, struct mutex *already)
{

	KASSERT(mutex_is_locked(already));
	mutex_lock(mutex);
}

static inline void
mutex_lock_nested(struct mutex *mutex, unsigned subclass __unused)
{
	mutex_lock(mutex);
}

static inline int
mutex_lock_interruptible_nested(struct mutex *mutex,
    unsigned subclass __unused)
{
	return mutex_lock_interruptible(mutex);
}

/*
 * `recursive locking is bad, do not use this ever.'
 * -- linux/scripts/checkpath.pl
 */
static inline enum {
	MUTEX_TRYLOCK_FAILED,
	MUTEX_TRYLOCK_SUCCESS,
	MUTEX_TRYLOCK_RECURSIVE,
}
mutex_trylock_recursive(struct mutex *mutex)
{
	if (mutex_owned(&mutex->mtx_lock))
		return MUTEX_TRYLOCK_RECURSIVE;
	else if (mutex_tryenter(&mutex->mtx_lock))
		return MUTEX_TRYLOCK_SUCCESS;
	else
		return MUTEX_TRYLOCK_FAILED;
}

#endif  /* _LINUX_MUTEX_H_ */
