/*	$NetBSD: pthread_specific.c,v 1.8 2003/05/15 19:16:37 wiz Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: pthread_specific.c,v 1.8 2003/05/15 19:16:37 wiz Exp $");

/* Functions and structures dealing with thread-specific data */
#include <errno.h>

#include "pthread.h"
#include "pthread_int.h"

static pthread_mutex_t tsd_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextkey;
int pthread__tsd_alloc[PTHREAD_KEYS_MAX];
void (*pthread__tsd_destructors[PTHREAD_KEYS_MAX])(void *);

__strong_alias(__libc_thr_keycreate,pthread_key_create)
__strong_alias(__libc_thr_setspecific,pthread_setspecific)
__strong_alias(__libc_thr_getspecific,pthread_getspecific)
__strong_alias(__libc_thr_keydelete,pthread_key_delete)

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int i;

	/* Get a lock on the allocation list */
	pthread_mutex_lock(&tsd_mutex);
	
	/* Find an avaliable slot */
	/* 1. Search from "nextkey" to the end of the list. */
	for (i = nextkey; i < PTHREAD_KEYS_MAX; i++)
		if (pthread__tsd_alloc[i] == 0)
			break;

	if (i == PTHREAD_KEYS_MAX) {
		/* 2. If that didn't work, search from the start
		 *    of the list back to "nextkey".
		 */
		for (i = 0; i < nextkey; i++)
			if (pthread__tsd_alloc[i] == 0)
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
	pthread__tsd_alloc[i] = 1;
	nextkey = (i + 1) % PTHREAD_KEYS_MAX;
	pthread__tsd_destructors[i] = destructor;
	pthread_mutex_unlock(&tsd_mutex);
	*key = i;

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
	 * http://groups.google.com/groups?hl=en&selm=u97d8.29%24fL6.200%40news.cpqcorp.net
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

	/* For the momemt, we're going with option 1. */
	pthread_mutex_lock(&tsd_mutex);
	pthread__tsd_destructors[key] = NULL;
	pthread_mutex_unlock(&tsd_mutex);

	return 0;
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
	pthread_t self;

	if (pthread__tsd_alloc[key] == 0)
		return EINVAL;

	self = pthread__self();
	/*
	 * We can't win here on constness. Having been given a 
	 * "const void *", we can only assign it to other const void *,
	 * and return it from functions that are const void *, without
	 * generating a warning. 
	 */
	/*LINTED const cast*/
	self->pt_specific[key] = (void *) value;

	return 0;
}

void*
pthread_getspecific(pthread_key_t key)
{
	pthread_t self;

	self = pthread__self();
	return (self->pt_specific[key]);
}

/* Perform thread-exit-time destruction of thread-specific data. */
void
pthread__destroy_tsd(pthread_t self)
{
	int i, done, iterations;
	void *val;
	void (*destructor)(void *);

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

	iterations = PTHREAD_DESTRUCTOR_ITERATIONS;
	do {
		done = 1;
		for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
			if (self->pt_specific[i] != NULL) {
				pthread_mutex_lock(&tsd_mutex);
				destructor = pthread__tsd_destructors[i];
				pthread_mutex_unlock(&tsd_mutex);
			    if (destructor != NULL) {
				    done = 0;
				    val = self->pt_specific[i];
				    self->pt_specific[i] = NULL; /* see above */
				    (*destructor)(val);
			    }
			}
		}
	} while (!done && iterations--);
}
