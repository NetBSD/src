/*	$NetBSD: pthread_atfork.c,v 1.27 2025/04/09 22:10:59 kre Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: pthread_atfork.c,v 1.27 2025/04/09 22:10:59 kre Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include "extern.h"
#include "reentrant.h"

#ifdef __weak_alias
__weak_alias(pthread_atfork, _pthread_atfork)
__weak_alias(fork, _fork)
#endif /* __weak_alias */

pid_t
__locked_fork(int *my_errno)
{
	return __fork();
}

struct atfork_callback {
	SIMPLEQ_ENTRY(atfork_callback) next;
	void (*fn)(void);
};

struct atfork_cb_header {
	uint16_t	entries;
	uint16_t	used;
};

struct atfork_cb_block {
	union {
		struct atfork_callback block;
		struct atfork_cb_header hdr;
	} u;
};

#define	cb_blocks(bp)	(&(bp)->u.block)
#define	cb_ents(bp)	(bp)->u.hdr.entries
#define	cb_used(bp)	(bp)->u.hdr.used

/*
 * We need to keep a cache for of at least 6, one for prepare, one for parent,
 * one for child x 2 bexause of the two uses in the libpthread (pthread_init,
 * pthread_tsd_init) constructors, where it is too early to call malloc(3).
 * This does not guarantee that we will have enough, because other libraries
 * can also call pthread_atfork() from their own constructors, so this is not
 * a complete solution and will need to be fixed properly. For now a keep
 * space for 16 since it is just 256 bytes.
 */
static struct atfork_callback atfork_builtin[16];
static struct atfork_cb_block *atfork_storage = NULL;
static int hw_pagesize = 0;

static const int hw_pagesize_sysctl[2] = { CTL_HW, HW_PAGESIZE };

/*
 * Hypothetically, we could protect the queues with a rwlock which is
 * write-locked by pthread_atfork() and read-locked by fork(), but
 * since the intended use of the functions is obtaining locks to hold
 * across the fork, forking is going to be serialized anyway.
 */
#ifdef _REENTRANT
static mutex_t atfork_lock = MUTEX_INITIALIZER;
#endif
SIMPLEQ_HEAD(atfork_callback_q, atfork_callback);

static struct atfork_callback_q prepareq = SIMPLEQ_HEAD_INITIALIZER(prepareq);
static struct atfork_callback_q parentq = SIMPLEQ_HEAD_INITIALIZER(parentq);
static struct atfork_callback_q childq = SIMPLEQ_HEAD_INITIALIZER(childq);

/*
 * Nb: nothing allocated by this allocator is ever freed.
 * (there is no API to free anything, and no need for one)
 *
 * The code relies upon this.
 */
static struct atfork_callback *
af_alloc(unsigned int blocks)
{
	struct atfork_callback *result;

	if (__predict_false(blocks == 0))
		return NULL;

	if (__predict_true(atfork_storage == NULL)) {
		for (size_t i = 0; i < __arraycount(atfork_builtin); i++) {
			if (atfork_builtin[i].fn == NULL) {
				if (i + blocks <= __arraycount(atfork_builtin))
					return &atfork_builtin[i];
				else
					break;
			}
		}
	}

	if (__predict_false(atfork_storage == NULL ||
	    cb_used(atfork_storage) + blocks > cb_ents(atfork_storage))) {
		if (__predict_false(hw_pagesize == 0)) {
			size_t len = sizeof(hw_pagesize);

			if (sysctl(hw_pagesize_sysctl, 2, &hw_pagesize,
			    &len, NULL, 0) != 0)
				return NULL;
			if (len != sizeof(hw_pagesize))
				return NULL;
			if (hw_pagesize == 0 || (hw_pagesize & 0xFF) != 0)
				return NULL;
		}
		atfork_storage = mmap(0, hw_pagesize, PROT_READ|PROT_WRITE,
		    MAP_PRIVATE | MAP_ANON, -1, 0);
		if (__predict_false(atfork_storage == NULL))
			return NULL;
		cb_used(atfork_storage) = 1;
		cb_ents(atfork_storage) =
		    (uint16_t)(hw_pagesize / sizeof(struct atfork_cb_block));
		if (__predict_false(cb_ents(atfork_storage) < blocks + 1))
			return NULL;
	}

	result = cb_blocks(atfork_storage) + cb_used(atfork_storage);
	cb_used(atfork_storage) += blocks;

	return result;
}

int
pthread_atfork(void (*prepare)(void), void (*parent)(void),
    void (*child)(void))
{
	struct atfork_callback *newprepare, *newparent, *newchild;
	sigset_t mask, omask;
	int error;

	sigfillset(&mask);
	thr_sigsetmask(SIG_SETMASK, &mask, &omask);

	mutex_lock(&atfork_lock);

	/*
	 * Note here that we either get all the blocks
	 * we need, in one call, or we get NULL.
	 *
	 * Note also that a NULL return is not an error
	 * if no blocks were required (all args == NULL)
	 */
	newprepare = af_alloc((prepare != NULL) +
	    (parent != NULL) + (child != NULL));

	error = ENOMEM;		/* in case of "goto out" */

	newparent = newprepare;
	if (prepare != NULL) {
		if (__predict_false(newprepare == NULL))
			goto out;
		newprepare->fn = prepare;
		newparent++;
	}

	newchild = newparent;
	if (parent != NULL) {
		if (__predict_false(newparent == NULL))
			goto out;
		newparent->fn = parent;
		newchild++;
	}

	if (child != NULL) {
		if (__predict_false(newchild == NULL))
			goto out;
		newchild->fn = child;
	}

	/*
	 * The order in which the functions are called is specified as
	 * LIFO for the prepare handler and FIFO for the others; insert
	 * at the head and tail as appropriate so that SIMPLEQ_FOREACH()
	 * produces the right order.
	 */
	if (prepare)
		SIMPLEQ_INSERT_HEAD(&prepareq, newprepare, next);
	if (parent)
		SIMPLEQ_INSERT_TAIL(&parentq, newparent, next);
	if (child)
		SIMPLEQ_INSERT_TAIL(&childq, newchild, next);

	error = 0;

 out:;
	mutex_unlock(&atfork_lock);
	thr_sigsetmask(SIG_SETMASK, &omask, NULL);
	return error;
}

pid_t
fork(void)
{
	struct atfork_callback *iter;
	pid_t ret;

	mutex_lock(&atfork_lock);
	SIMPLEQ_FOREACH(iter, &prepareq, next)
		(*iter->fn)();
	_malloc_prefork();

	ret = __locked_fork(&errno);

	if (ret != 0) {
		/*
		 * We are the parent. It doesn't matter here whether
		 * the fork call succeeded or failed.
		 */
		_malloc_postfork();
		SIMPLEQ_FOREACH(iter, &parentq, next)
			(*iter->fn)();
		mutex_unlock(&atfork_lock);
	} else {
		/* We are the child */
		_malloc_postfork_child();
		SIMPLEQ_FOREACH(iter, &childq, next)
			(*iter->fn)();
		/*
		 * Note: We are explicitly *not* unlocking
		 * atfork_lock.  Unlocking atfork_lock is problematic,
		 * because if any threads in the parent blocked on it
		 * between the initial lock and the fork() syscall,
		 * unlocking in the child will try to schedule
		 * threads, and either the internal mutex interlock or
		 * the runqueue spinlock could have been held at the
		 * moment of fork(). Since the other threads do not
		 * exist in this process, the spinlock will never be
		 * unlocked, and we would wedge.
		 * Instead, we reinitialize atfork_lock, since we know
		 * that the state of the atfork lists is consistent here,
		 * and that there are no other threads to be affected by
		 * the forcible cleaning of the queue.
		 * This permits double-forking to work, although
		 * it requires knowing that it's "safe" to initialize
		 * a locked mutex in this context.
		 *
		 * The problem exists for users of this interface,
		 * too, since the intended use of pthread_atfork() is
		 * to acquire locks across the fork call to ensure
		 * that the child sees consistent state. There's not
		 * much that can usefully be done in a child handler,
		 * and conventional wisdom discourages using them, but
		 * they're part of the interface, so here we are...
		 */
		mutex_init(&atfork_lock, NULL);
	}

	return ret;
}
