/*	$NetBSD: pthread-stub.c,v 1.1 1999/11/14 18:34:15 explorer Exp $	*/

/*-
 * Copyright (c) 1999 Michael Graff <explorer@flame.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Michael Graff.
 * 3. Neither the name of the author nor the names of contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "namespace.h"

#include <pthread.h>

#include <stdio.h>

#ifdef __weak_alias
__weak_alias(pthread_mutex_init, _pthread_mutex_init);
__weak_alias(pthread_mutex_lock, _pthread_mutex_lock);
__weak_alias(pthread_mutex_unlock, _pthread_mutex_unlock);

__weak_alias(pthread_rwlock_init, _pthread_rwlock_init);
__weak_alias(pthread_rwlock_rdlock, _pthread_rwlock_rdlock);
__weak_alias(pthread_rwlock_wrlock, _pthread_rwlock_wrlock);
__weak_alias(pthread_rwlock_unlock, _pthread_rwlock_unlock);

__weak_alias(flockfile, _flockfile);
__weak_alias(funlockfile, _funlockfile);
#endif

int _pthread_mutex_init(pthread_mutex_t *, pthread_mutexattr_t *);
int _pthread_mutex_lock(pthread_mutex_t *);
int _pthread_mutex_unlock(pthread_mutex_t *);

int _pthread_rwlock_init(pthread_rwlock_t *, pthread_rwlockattr_t *);
int _pthread_rwlock_rdlock(pthread_rwlock_t *);
int _pthread_rwlock_wrlock(pthread_rwlock_t *);
int _pthread_rwlock_unlock(pthread_rwlock_t *);

int _flockfile(FILE *);
int _funlockfile(FILE *);

int
_pthread_mutex_init(pthread_mutex_t *m, pthread_mutexattr_t *a)
{
	(void)m;
	(void)a;

	return (0);
}

int
_pthread_mutex_lock(pthread_mutex_t *m)
{
	(void)m;

	return (0);
}

int
_pthread_mutex_unlock(pthread_mutex_t *m)
{
	(void)m;

	return (0);
}

int
_pthread_rwlock_init(pthread_rwlock_t *l, pthread_rwlockattr_t *a)
{
	(void)l;
	(void)a;

	return (0);
}

int
_pthread_rwlock_rdlock(pthread_rwlock_t *l)
{
	(void)l;

	return (0);
}

int
_pthread_rwlock_wrlock(pthread_rwlock_t *l)
{
	(void)l;

	return (0);
}

int
_pthread_rwlock_unlock(pthread_rwlock_t *l)
{
	(void)l;

	return (0);
}

int
_flockfile(FILE *fp)
{
	(void)fp;

	return (0);
}

int
_funlockfile(FILE *fp)
{
	(void)fp;

	return (0);
}
