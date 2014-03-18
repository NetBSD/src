/*	$NetBSD: mutex.h,v 1.2 2014/03/18 18:20:43 riastradh Exp $	*/

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

struct mutex {
	kmutex_t mtx_lock;
};

/* Name collision.  Pooh.  */
static inline void
linux_mutex_init(struct mutex *mutex)
{
	mutex_init(&mutex->mtx_lock, MUTEX_DEFAULT, IPL_NONE);
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

#endif  /* _LINUX_MUTEX_H_ */
