/*	$NetBSD: pthread_libcstubs.c,v 1.1.2.1 2002/04/11 02:53:09 nathanw Exp $	*/

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

#include "pthread.h"
#include "pthread_int.h"

/*
 * Interfaces for use by libc; libc uses these rather than the
 * standard interfaces since it different pthread libraries are not
 * ABI-compatible.
 */

int
_libc_pthread_mutex_init(pthread_mutex_t *mutex,
    const pthread_mutexattr_t *attr)
{
	return pthread_mutex_init(mutex, attr);
}


int
_libc_pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}


int
_libc_pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	return pthread_mutex_trylock(mutex);
}


int
_libc_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}


int
_libc_pthread_cond_init(pthread_cond_t *cond,
    const pthread_condattr_t *attr)
{
	return pthread_cond_init(cond, attr);
}


int
_libc_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	return pthread_cond_wait(cond, mutex);
}


int
_libc_pthread_cond_signal(pthread_cond_t *cond)
{
	return pthread_cond_signal(cond);
}


int
_libc_pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	return pthread_key_create(key, destructor);
}


int
_libc_pthread_key_delete(pthread_key_t key)
{
	return pthread_key_delete(key);
}


int
_libc_pthread_setspecific(pthread_key_t key, const void *value)
{
	return pthread_setspecific(key, value);
}


void*
_libc_pthread_getspecific(pthread_key_t key)
{
	return pthread_getspecific(key);
}


int
_libc_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	return pthread_sigmask(how, set, oset);
}


pthread_t
_libc_pthread_self(void)
{
	return pthread__self();
}


void
_libc_pthread_exit(void *retval)
{
	pthread_exit(retval);
}


int
_libc_pthread_once(pthread_once_t *once_control, void (*routine)(void))
{
	return pthread_once(once_control, routine);
}

int *
_libc_pthread__errno(void)
{
	return pthread__errno();
}
