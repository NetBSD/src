/*	$NetBSD: pthread_specific.c,v 1.1.2.6 2002/04/26 22:10:49 nathanw Exp $	*/

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

/* Functions and structures dealing with thread-specific data */
#include <assert.h>
#include <errno.h>
#include <sys/cdefs.h>

#include "pthread.h"
#include "pthread_int.h"

static pthread_mutex_t tsd_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextkey;
int pthread__tsd_alloc[PTHREAD_KEYS_MAX];
void (*pthread__tsd_destructors[PTHREAD_KEYS_MAX])(void *);

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
	pthread_mutex_unlock(&tsd_mutex);
	pthread__tsd_destructors[i] = destructor;
	*key = i;

	return 0;
}

int
pthread_key_delete(pthread_key_t key)
{
	pthread_t self, thread;
	extern pthread_spin_t allqueue_lock;
	extern struct pthread_queue_t allqueue;

	/* This takes a little work. When pthread_key_create() returns
	 * a new key, the data for that key must be NULL in all threads.
	 * To make pthread_key_create() easy, we want to nuke all non-NULL
	 * values here. This means we have to iterate the list of all
	 * threads. New threads aren't a problem, because the thread setup
	 * will zero out their TSD array. We just have to keep the thread
	 * under examiniation from being yanked out from under us.
	 */

	self = pthread__self();
	/* XXX this is a long operation to protect with a spinlock */
	pthread_spinlock(self, &allqueue_lock);
	/* XXX this is a memory leak; standards-compliant, but
	 * pthread_key_delete() is not a well-thought-out part of the
	 * standard. Reconsider.
	 * (well, it could be worse. we could be running destructors here,
	 * which would be a total mess.)
	 * See David Butenhof's article in comp.programming.threads:
	 * Subject: Re: TSD key reusing issue
	 * Message-ID: <u97d8.29$fL6.200@news.cpqcorp.net>
	 * Date: Thu, 21 Feb 2002 09:06:17 -0500
	 * http://groups.google.com/groups?hl=en&selm=u97d8.29%24fL6.200%40news.cpqcorp.net
	 */
	PTQ_FOREACH(thread, &allqueue, pt_allq)
		thread->pt_specific[key] = NULL;
	pthread_spinunlock(self, &allqueue_lock);

	pthread__tsd_destructors[key] = NULL;
	pthread_mutex_lock(&tsd_mutex);
	pthread__tsd_alloc[key] = 0;
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
	/* We can't win here on constness. Having been given a 
	 * "const void *", we can only assign it to other const void *,
	 * and return it from functions that are const void *, without
	 * generating a warning. 
	 */
	self->pt_specific[key] = (void *) value;

	return 0;
}

void*
pthread_getspecific(pthread_key_t key)
{
	pthread_t self;

	if (pthread__tsd_alloc[key] == 0)
		return NULL;

	self = pthread__self();
	return (self->pt_specific[key]);
}

/* Perform thread-exit-time destruction of thread-specific data. */
void
pthread__destroy_tsd(pthread_t self)
{
	int i, done, iterations;
	void *val;

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
			if ((self->pt_specific[i] != NULL) &&
			    pthread__tsd_destructors[i] != NULL) {
				done = 0;
				val = self->pt_specific[i];
				self->pt_specific[i] = NULL; /* see above */
				pthread__tsd_destructors[i](val);
			}
		}
	} while (!done && iterations--);
}
