/*	$NetBSD: pthread_spin.c,v 1.9 2022/02/12 14:59:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Andrew Doran.
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
 * Public (POSIX-specified) spinlocks.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_spin.c,v 1.9 2022/02/12 14:59:32 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

#include <sys/types.h>
#include <sys/ras.h>

#include <machine/lock.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pthread.h"
#include "pthread_int.h"

int
pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{

	pthread__error(EINVAL, "Invalid pshared",
	    pshared == PTHREAD_PROCESS_PRIVATE ||
	    pshared == PTHREAD_PROCESS_SHARED);

	lock->pts_magic = _PT_SPINLOCK_MAGIC;

	/*
	 * We don't actually use the pshared flag for anything;
	 * CPU simple locks have all the process-shared properties 
	 * that we want anyway.
	 */
	lock->pts_flags = pshared;
	pthread_lockinit(&lock->pts_spin);

	return 0;
}

int
pthread_spin_destroy(pthread_spinlock_t *lock)
{

	pthread__error(EINVAL, "Invalid spinlock",
	    lock->pts_magic == _PT_SPINLOCK_MAGIC);

	if (!__SIMPLELOCK_UNLOCKED_P(&lock->pts_spin))
		return EBUSY;

	lock->pts_magic = _PT_SPINLOCK_DEAD;

	return 0;
}

int
pthread_spin_lock(pthread_spinlock_t *lock)
{
	pthread_t self;

	pthread__error(EINVAL, "Invalid spinlock",
	    lock->pts_magic == _PT_SPINLOCK_MAGIC);

	self = pthread__self();
	while (pthread__spintrylock(self, &lock->pts_spin) == 0) {
		pthread__smt_pause();
	}

	return 0;
}

int
pthread_spin_trylock(pthread_spinlock_t *lock)
{
	pthread_t self;

	pthread__error(EINVAL, "Invalid spinlock",
	    lock->pts_magic == _PT_SPINLOCK_MAGIC);

	self = pthread__self();
	if (pthread__spintrylock(self, &lock->pts_spin) == 0)
		return EBUSY;
	return 0;
}

int
pthread_spin_unlock(pthread_spinlock_t *lock)
{
	pthread_t self;

	pthread__error(EINVAL, "Invalid spinlock",
	    lock->pts_magic == _PT_SPINLOCK_MAGIC);

	self = pthread__self();
	pthread__spinunlock(self, &lock->pts_spin);
	pthread__smt_wake();

	return 0;
}
