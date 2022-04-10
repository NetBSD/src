/*	$NetBSD: pthread_tsd.c,v 1.25 2022/04/10 10:38:33 riastradh Exp $	*/

/*-
 * Copyright (c) 2001, 2007, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, by Andrew Doran, and by Christos Zoulas.
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
__RCSID("$NetBSD: pthread_tsd.c,v 1.25 2022/04/10 10:38:33 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

/* Functions and structures dealing with thread-specific data */
#include <errno.h>
#include <sys/mman.h>

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"
#include "tsd.h"

int pthread_keys_max;
static pthread_mutex_t tsd_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextkey;

PTQ_HEAD(pthread__tsd_list, pt_specific) *pthread__tsd_list = NULL;
void (**pthread__tsd_destructors)(void *) = NULL;

__strong_alias(__libc_thr_keycreate,pthread_key_create)
__strong_alias(__libc_thr_keydelete,pthread_key_delete)

static void
/*ARGSUSED*/
null_destructor(void *p)
{
}

#include <err.h>
#include <stdlib.h>
#include <stdio.h>

static void
pthread_tsd_prefork(void)
{
	pthread_mutex_lock(&tsd_mutex);
}

static void
pthread_tsd_postfork(void)
{
	pthread_mutex_unlock(&tsd_mutex);
}

static void
pthread_tsd_postfork_child(void)
{
	pthread_mutex_init(&tsd_mutex, NULL);
}

void *
pthread_tsd_init(size_t *tlen)
{
	char *pkm;
	size_t alen;
	char *arena;

	pthread_atfork(pthread_tsd_prefork, pthread_tsd_postfork, pthread_tsd_postfork_child);

	if ((pkm = pthread__getenv("PTHREAD_KEYS_MAX")) != NULL) {
		pthread_keys_max = (int)strtol(pkm, NULL, 0);
		if (pthread_keys_max < _POSIX_THREAD_KEYS_MAX)
			pthread_keys_max = _POSIX_THREAD_KEYS_MAX;
	} else {
		pthread_keys_max = PTHREAD_KEYS_MAX;
	}

	/*
	 * Can't use malloc here yet, because malloc will use the fake
	 * libc thread functions to initialize itself, so mmap the space.
	 */
	*tlen = sizeof(struct __pthread_st)
	    + pthread_keys_max * sizeof(struct pt_specific);
	alen = *tlen
	    + sizeof(*pthread__tsd_list) * pthread_keys_max
	    + sizeof(*pthread__tsd_destructors) * pthread_keys_max;

	arena = mmap(NULL, alen, PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
	if (arena == MAP_FAILED) {
		pthread_keys_max = 0;
		return NULL;
	}

	pthread__tsd_list = (void *)arena;
	arena += sizeof(*pthread__tsd_list) * pthread_keys_max;
	pthread__tsd_destructors = (void *)arena;
	arena += sizeof(*pthread__tsd_destructors) * pthread_keys_max;
	return arena;
}

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int i;

	if (__predict_false(__uselibcstub))
		return __libc_thr_keycreate_stub(key, destructor);

	/* Get a lock on the allocation list */
	pthread_mutex_lock(&tsd_mutex);

	/* Find an available slot:
	 * The condition for an available slot is one with the destructor
	 * not being NULL. If the desired destructor is NULL we set it to
	 * our own internal destructor to satisfy the non NULL condition.
	 */
	/* 1. Search from "nextkey" to the end of the list. */
	for (i = nextkey; i < pthread_keys_max; i++)
		if (pthread__tsd_destructors[i] == NULL)
			break;

	if (i == pthread_keys_max) {
		/* 2. If that didn't work, search from the start
		 *    of the list back to "nextkey".
		 */
		for (i = 0; i < nextkey; i++)
			if (pthread__tsd_destructors[i] == NULL)
				break;

		if (i == nextkey) {
			/* If we didn't find one here, there isn't one
			 * to be found.
			 */
			pthread_mutex_unlock(&tsd_mutex);
			return EAGAIN;
		}
	}

	/* Got one. */
	pthread__assert(PTQ_EMPTY(&pthread__tsd_list[i]));
	pthread__tsd_destructors[i] = destructor ? destructor : null_destructor;

	nextkey = (i + 1) % pthread_keys_max;
	pthread_mutex_unlock(&tsd_mutex);
	*key = i;

	return 0;
}

/*
 * Each thread holds an array of pthread_keys_max pt_specific list
 * elements. When an element is used it is inserted into the appropriate
 * key bucket of pthread__tsd_list. This means that ptqe_prev == NULL,
 * means that the element is not threaded, ptqe_prev != NULL it is
 * already part of the list. If a key is set to a non-NULL value for the
 * first time, it is added to the list.
 *
 * We keep this global array of lists of threads that have called
 * pthread_set_specific with non-null values, for each key so that
 * we don't have to check all threads for non-NULL values in
 * pthread_key_destroy.
 *
 * The assumption here is that a concurrent pthread_key_delete is already
 * undefined behavior. The mutex is taken only once per thread/key
 * combination.
 *
 * We could keep an accounting of the number of specific used
 * entries per thread, so that we can update pt_havespecific when we delete
 * the last one, but we don't bother for now
 */
int
pthread__add_specific(pthread_t self, pthread_key_t key, const void *value)
{
	struct pt_specific *pt;

	pthread__assert(key >= 0 && key < pthread_keys_max);

	pthread__assert(pthread__tsd_destructors[key] != NULL);
	pt = &self->pt_specific[key];
	self->pt_havespecific = 1;
	if (value && !pt->pts_next.ptqe_prev) {
		pthread_mutex_lock(&tsd_mutex);
		PTQ_INSERT_HEAD(&pthread__tsd_list[key], pt, pts_next);
		pthread_mutex_unlock(&tsd_mutex);
	}
	pt->pts_value = __UNCONST(value);

	return 0;
}

int
pthread_key_delete(pthread_key_t key)
{
	/*
	 * This is tricky.  The standard says of pthread_key_create()
	 * that new keys have the value NULL associated with them in
	 * all threads.  According to people who were present at the
	 * standardization meeting, that requirement was written
	 * before pthread_key_delete() was introduced, and not
	 * reconsidered when it was.
	 *
	 * See David Butenhof's article in comp.programming.threads:
	 * Subject: Re: TSD key reusing issue
	 * Message-ID: <u97d8.29$fL6.200@news.cpqcorp.net>
	 * Date: Thu, 21 Feb 2002 09:06:17 -0500
	 *	 http://groups.google.com/groups?\
	 *	 hl=en&selm=u97d8.29%24fL6.200%40news.cpqcorp.net
	 *
	 * Given:
	 *
	 * 1: Applications are not required to clear keys in all
	 *    threads before calling pthread_key_delete().
	 * 2: Clearing pointers without running destructors is a
	 *    memory leak.
	 * 3: The pthread_key_delete() function is expressly forbidden
	 *    to run any destructors.
	 *
	 * Option 1: Make this function effectively a no-op and
	 * prohibit key reuse. This is a possible resource-exhaustion
	 * problem given that we have a static storage area for keys,
	 * but having a non-static storage area would make
	 * pthread_setspecific() expensive (might need to realloc the
	 * TSD array).
	 *
	 * Option 2: Ignore the specified behavior of
	 * pthread_key_create() and leave the old values. If an
	 * application deletes a key that still has non-NULL values in
	 * some threads... it's probably a memory leak and hence
	 * incorrect anyway, and we're within our rights to let the
	 * application lose. However, it's possible (if unlikely) that
	 * the application is storing pointers to non-heap data, or
	 * non-pointers that have been wedged into a void pointer, so
	 * we can't entirely write off such applications as incorrect.
	 * This could also lead to running (new) destructors on old
	 * data that was never supposed to be associated with that
	 * destructor.
	 *
	 * Option 3: Follow the specified behavior of
	 * pthread_key_create().  Either pthread_key_create() or
	 * pthread_key_delete() would then have to clear the values in
	 * every thread's slot for that key. In order to guarantee the
	 * visibility of the NULL value in other threads, there would
	 * have to be synchronization operations in both the clearer
	 * and pthread_getspecific().  Putting synchronization in
	 * pthread_getspecific() is a big performance lose.  But in
	 * reality, only (buggy) reuse of an old key would require
	 * this synchronization; for a new key, there has to be a
	 * memory-visibility propagating event between the call to
	 * pthread_key_create() and pthread_getspecific() with that
	 * key, so setting the entries to NULL without synchronization
	 * will work, subject to problem (2) above. However, it's kind
	 * of slow.
	 *
	 * Note that the argument in option 3 only applies because we
	 * keep TSD in ordinary memory which follows the pthreads
	 * visibility rules. The visibility rules are not required by
	 * the standard to apply to TSD, so the argument doesn't
	 * apply in general, just to this implementation.
	 */

	/*
	 * We do option 3; we find the list of all pt_specific structures
	 * threaded on the key we are deleting, unthread them, and set the
	 * pointer to NULL. Finally we unthread the entry, freeing it for
	 * further use.
	 *
	 * We don't call the destructor here, it is the responsibility
	 * of the application to cleanup the storage:
	 * 	http://pubs.opengroup.org/onlinepubs/9699919799/functions/\
	 *	pthread_key_delete.html
	 */
	struct pt_specific *pt;

	if (__predict_false(__uselibcstub))
		return __libc_thr_keydelete_stub(key);

	pthread__assert(key >= 0 && key < pthread_keys_max);

	pthread_mutex_lock(&tsd_mutex);

	pthread__assert(pthread__tsd_destructors[key] != NULL);

	while ((pt = PTQ_FIRST(&pthread__tsd_list[key])) != NULL) {
		PTQ_REMOVE(&pthread__tsd_list[key], pt, pts_next);
		pt->pts_value = NULL;
		pt->pts_next.ptqe_prev = NULL;
	}

	pthread__tsd_destructors[key] = NULL;
	pthread_mutex_unlock(&tsd_mutex);

	return 0;
}

/* Perform thread-exit-time destruction of thread-specific data. */
void
pthread__destroy_tsd(pthread_t self)
{
	int i, done, iterations;
	void *val;
	void (*destructor)(void *);

	if (!self->pt_havespecific)
		return;

	/* Butenhof, section 5.4.2 (page 167):
	 *
	 * ``Also, Pthreads sets the thread-specific data value for a
	 * key to NULL before calling that key's destructor (passing
	 * the previous value of the key) when a thread terminates [*].
	 * ...
	 * [*] That is, unfortunately, not what the standard
	 * says. This is one of the problems with formal standards -
	 * they say what they say, not what they were intended to
	 * say. Somehow, an error crept in, and the sentence
	 * specifying that "the implementation clears the
	 * thread-specific data value before calling the destructor"
	 * was deleted. Nobody noticed, and the standard was approved
	 * with the error. So the standard says (by omission) that if
	 * you want to write a portable application using
	 * thread-specific data, that will not hang on thread
	 * termination, you must call pthread_setspecific within your
	 * destructor function to change the value to NULL. This would
	 * be silly, and any serious implementation of Pthreads will
	 * violate the standard in this respect. Of course, the
	 * standard will be fixed, probably by the 1003.1n amendment
	 * (assorted corrections to 1003.1c-1995), but that will take
	 * a while.''
	 */

	/* We're not required to try very hard */
	iterations = PTHREAD_DESTRUCTOR_ITERATIONS;
	do {
		done = 1;
		for (i = 0; i < pthread_keys_max; i++) {
			struct pt_specific *pt = &self->pt_specific[i];
			if (pt->pts_next.ptqe_prev == NULL)
				continue;
			pthread_mutex_lock(&tsd_mutex);

			if (pt->pts_next.ptqe_prev != NULL)  {
				PTQ_REMOVE(&pthread__tsd_list[i], pt, pts_next);
				val = pt->pts_value;
				pt->pts_value = NULL;
				pt->pts_next.ptqe_prev = NULL;
				destructor = pthread__tsd_destructors[i];
			} else
				destructor = NULL;

			pthread_mutex_unlock(&tsd_mutex);
			if (destructor != NULL && val != NULL) {
				done = 0;
				(*destructor)(val);
			}
		}
	} while (!done && --iterations);

	self->pt_havespecific = 0;
}

void
pthread__copy_tsd(pthread_t self)
{
	for (size_t key = 0; key < TSD_KEYS_MAX; key++) {

		if (__libc_tsd[key].tsd_inuse == 0)
			continue;

		pthread__assert(pthread__tsd_destructors[key] == NULL);
		pthread__tsd_destructors[key] = __libc_tsd[key].tsd_dtor ?
		    __libc_tsd[key].tsd_dtor : null_destructor;
		nextkey = (key + 1) % pthread_keys_max;

		self->pt_havespecific = 1;
		struct pt_specific *pt = &self->pt_specific[key];
		pt->pts_value = __libc_tsd[key].tsd_val;
		__libc_tsd[key].tsd_inuse = 0;
	}
}
