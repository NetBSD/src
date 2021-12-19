/*	$NetBSD: linux_rwsem.c,v 1.1 2021/12/19 11:21:30 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_rwsem.c,v 1.1 2021/12/19 11:21:30 riastradh Exp $");

#include <sys/types.h>

#include <sys/condvar.h>
#include <sys/lwp.h>
#include <sys/rwlock.h>

#include <machine/limits.h>

#include <lib/libkern/libkern.h>

#include <linux/rwsem.h>

void
init_rwsem(struct rw_semaphore *rwsem)
{

	mutex_init(&rwsem->rws_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&rwsem->rws_cv, "lnxrwsem");
	rwsem->rws_writer = NULL;
	rwsem->rws_readers = 0;
}

void
destroy_rwsem(struct rw_semaphore *rwsem)
{

	KASSERT(rwsem->rws_readers == 0);
	KASSERT(rwsem->rws_writer == NULL);
	cv_destroy(&rwsem->rws_cv);
	mutex_destroy(&rwsem->rws_lock);
}

void
down_read(struct rw_semaphore *rwsem)
{

	mutex_enter(&rwsem->rws_lock);
	while (rwsem->rws_writer || rwsem->rws_writewanted)
		cv_wait(&rwsem->rws_cv, &rwsem->rws_lock);
	KASSERT(rwsem->rws_readers < UINT_MAX);
	rwsem->rws_readers++;
	mutex_exit(&rwsem->rws_lock);
}

bool
down_read_trylock(struct rw_semaphore *rwsem)
{
	bool ret = false;

	/*
	 * Note: Linux apparently relies on down_read_trylock to
	 * quietly succeed when the caller already holds a reader lock.
	 * This is why we can't use rwlock(9), which absolutely
	 * prohibits recursive use and crashes immediately under
	 * LOCKDEBUG if you try it.
	 */

	mutex_enter(&rwsem->rws_lock);
	if (rwsem->rws_writer == NULL && !rwsem->rws_writewanted) {
		KASSERT(rwsem->rws_readers < UINT_MAX);
		rwsem->rws_readers++;
		ret = true;
	}
	mutex_exit(&rwsem->rws_lock);

	return ret;
}

void
up_read(struct rw_semaphore *rwsem)
{

	mutex_enter(&rwsem->rws_lock);
	KASSERT(rwsem->rws_readers);
	KASSERT(rwsem->rws_writer == NULL);
	if (--rwsem->rws_readers == 0)
		cv_broadcast(&rwsem->rws_cv);
	mutex_exit(&rwsem->rws_lock);
}

void
down_write(struct rw_semaphore *rwsem)
{

	mutex_enter(&rwsem->rws_lock);

	/* If another writer is waiting, get in the queue.  */
	while (rwsem->rws_writewanted)
		cv_wait(&rwsem->rws_cv, &rwsem->rws_lock);

	/*
	 * No other writers waiting.  Our turn.  Announce our intent to
	 * readers, and wait for the writer or readers to finish.
	 */
	rwsem->rws_writewanted = true;
	while (rwsem->rws_writer || rwsem->rws_readers) {
		KASSERTMSG(rwsem->rws_writer != curlwp,
		    "locking against myself: rwsem=%p lwp=%p", rwsem, curlwp);
		cv_wait(&rwsem->rws_cv, &rwsem->rws_lock);
	}

	/* At last, it is ours!  */
	KASSERT(rwsem->rws_readers == 0);
	KASSERT(rwsem->rws_writer == NULL);
	KASSERT(rwsem->rws_writewanted);
	rwsem->rws_writewanted = false;
	rwsem->rws_writer = curlwp;

	mutex_exit(&rwsem->rws_lock);
}

void
up_write(struct rw_semaphore *rwsem)
{

	mutex_enter(&rwsem->rws_lock);
	KASSERT(rwsem->rws_writer == curlwp);
	KASSERT(rwsem->rws_readers == 0);
	rwsem->rws_writer = NULL;
	cv_broadcast(&rwsem->rws_cv);
	mutex_exit(&rwsem->rws_lock);
}

void
downgrade_write(struct rw_semaphore *rwsem)
{

	mutex_enter(&rwsem->rws_lock);
	KASSERT(rwsem->rws_writer == curlwp);
	KASSERT(rwsem->rws_readers == 0);
	rwsem->rws_writer = NULL;
	rwsem->rws_readers = 1;
	cv_broadcast(&rwsem->rws_cv);
	mutex_exit(&rwsem->rws_lock);
}
