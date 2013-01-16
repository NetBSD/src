/*	$NetBSD: pthread_tsd.c,v 1.7.4.2 2013/01/16 05:32:27 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_tsd.c,v 1.7.4.2 2013/01/16 05:32:27 yamt Exp $");

/* Functions and structures dealing with thread-specific data */
#include <errno.h>

#include "pthread.h"
#include "pthread_int.h"


static pthread_mutex_t tsd_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextkey;

PTQ_HEAD(pthread__tsd_list, pt_specific)
    pthread__tsd_list[PTHREAD_KEYS_MAX];
void (*pthread__tsd_destructors[PTHREAD_KEYS_MAX])(void *);

__strong_alias(__libc_thr_keycreate,pthread_key_create)
__strong_alias(__libc_thr_keydelete,pthread_key_delete)

static void
/*ARGSUSED*/
null_destructor(void *p)
{
}

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int i;

	/* Get a lock on the allocation list */
	pthread_mutex_lock(&tsd_mutex);
	
	/* Find an available slot:
	 * The condition for an available slot is one with the destructor
	 * not being NULL. If the desired destructor is NULL we set it to 
	 * our own internal destructor to satisfy the non NULL condition.
	 */
	/* 1. Search from "nextkey" to the end of the list. */
	for (i = nextkey; i < PTHREAD_KEYS_MAX; i++)
		if (pthread__tsd_destructors[i] == NULL)
			break;

	if (i == PTHREAD_KEYS_MAX) {
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

	nextkey = (i + 1) % PTHREAD_KEYS_MAX;
	pthread_mutex_unlock(&tsd_mutex);
	*key = i;

	return 0;
}

/*
 * Each thread holds an array of PTHREAD_KEYS_MAX pt_specific list
 * elements. When an element is used it is inserted into the appropriate
 * key bucket of pthread__tsd_list. This means that ptqe_prev == NULL,
 * means that the element is not threaded, ptqe_prev != NULL it is
 * already part of the list. When we set to a NULL value we delete from the
 * list if it was in the list, and when we set to non-NULL value, we insert
 * in the list if it was not already there.
 *
 * We keep this global array of lists of threads that have called
 * pthread_set_specific with non-null values, for each key so that
 * we don't have to check all threads for non-NULL values in
 * pthread_key_destroy
 *
 * We could keep an accounting of the number of specific used
 * entries per thread, so that we can update pt_havespecific when we delete
 * the last one, but we don't bother for now
 */
int
pthread__add_specific(pthread_t self, pthread_key_t key, const void *value)
{
	struct pt_specific *pt;

	pthread__assert(key >= 0 && key < PTHREAD_KEYS_MAX);

	pthread_mutex_lock(&tsd_mutex);
	pthread__assert(pthread__tsd_destructors[key] != NULL);
	pt = &self->pt_specific[key];
	self->pt_havespecific = 1;
	if (value) {
		if (pt->pts_next.ptqe_prev == NULL)
			PTQ_INSERT_HEAD(&pthread__tsd_list[key], pt, pts_next);
	} else {
		if (pt->pts_next.ptqe_prev != NULL) {
			PTQ_REMOVE(&pthread__tsd_list[key], pt, pts_next);
			pt->pts_next.ptqe_prev = NULL;
		}
	}
	pt->pts_value = __UNCONST(value);
	pthread_mutex_unlock(&tsd_mutex);

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

	pthread__assert(key >= 0 && key < PTHREAD_KEYS_MAX);

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
	pthread_mutex_unlock(&self->pt_lock);

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

	iterations = 4; /* We're not required to try very hard */
	do {
		done = 1;
		for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
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
			if (destructor != NULL) {
				done = 0;
				(*destructor)(val);
			}
		}
	} while (!done && iterations--);

	self->pt_havespecific = 0;
	pthread_mutex_lock(&self->pt_lock);
}
