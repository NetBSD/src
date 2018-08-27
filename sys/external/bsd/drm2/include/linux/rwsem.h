/*	$NetBSD: rwsem.h,v 1.2 2018/08/27 07:47:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

/*
 * These are not really semaphores; they are just reader/writer sleep
 * locks, whereas what Linux calls an rwlock is a reader/writer spin
 * lock.
 *
 * Notes on porting:
 *
 * - Linux does not have destroy_rwsem.  You must add it yourself in
 *   the appropriate place.
 */

#ifndef _LINUX_RWSEM_H_
#define _LINUX_RWSEM_H_

#include <sys/mutex.h>

struct rw_semaphore {
	krwlock_t	rws_lock;
};

static inline void
init_rwsem(struct rw_semaphore *rwsem)
{

	rw_init(&rwsem->rws_lock);
}

static inline void
destroy_rwsem(struct rw_semaphore *rwsem)
{

	rw_destroy(&rwsem->rws_lock);
}

static inline void
down_read(struct rw_semaphore *rwsem)
{

	rw_enter(&rwsem->rws_lock, RW_READER);
}

static inline bool
down_read_trylock(struct rw_semaphore *rwsem)
{

	return rw_tryenter(&rwsem->rws_lock, RW_READER);
}

static inline void
down_write(struct rw_semaphore *rwsem)
{

	rw_enter(&rwsem->rws_lock, RW_WRITER);
}

static inline void
up_read(struct rw_semaphore *rwsem)
{

	rw_exit(&rwsem->rws_lock);
}

static inline void
up_write(struct rw_semaphore *rwsem)
{

	rw_exit(&rwsem->rws_lock);
}

static inline void
downgrade_write(struct rw_semaphore *rwsem)
{

	rw_downgrade(&rwsem->rws_lock);
}

#endif  /* _LINUX_RWSEM_H_ */
