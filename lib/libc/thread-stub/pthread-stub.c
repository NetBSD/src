/*	$NetBSD: pthread-stub.c,v 1.3.6.11 2002/05/02 16:56:33 nathanw Exp $	*/

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
 *        This product includes software developed by Michael Graff
 *	  for the NetBSD Project, http://www.netbsd.org/
 * 3. Neither the name of the author nor the names of contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
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

static int
_pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)m;
	/* LINTED deliberate lack of effect */
	(void)a;

	return (0);
}

static int
_pthread_mutex_lock(pthread_mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)m;

	return (0);
}

static int
_pthread_mutex_unlock(pthread_mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)m;

	return (0);
}
#if 0
static int
_pthread_rwlock_init(pthread_rwlock_t *l, pthread_rwlockattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)l;
	/* LINTED deliberate lack of effect */
	(void)a;

	return (0);
}

static int
_pthread_rwlock_rdlock(pthread_rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	return (0);
}

static int
_pthread_rwlock_wrlock(pthread_rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	return (0);
}

static int
_pthread_rwlock_unlock(pthread_rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	return (0);
}
#endif

static int
_pthread_once(pthread_once_t *o, void (*r)(void))
{
	/* LINTED deliberate lack of effect */
	(void)o;
	/* LINTED deliberate lack of effect */
	(void)r;

	return (0);
}

static int
_pthread_cond_init(pthread_cond_t *c,const pthread_condattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)a;

	return (0);
}

static int
_pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)m;

	return (0);
}

static int
_pthread_cond_signal(pthread_cond_t *c)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	
	return (0);
}

static int
_pthread_key_create(pthread_key_t *k, void (*d)(void *))
{
	/* LINTED deliberate lack of effect */
	(void)k;
	/* LINTED deliberate lack of effect */
	(void)d;

	return (0);
}

static int
_pthread_setspecific(pthread_key_t k, const void *v)
{
	/* LINTED deliberate lack of effect */
	(void)k;
	/* LINTED deliberate lack of effect */
	(void)v;

	return (0);
}

static void*
_pthread_getspecific(pthread_key_t k)
{
	/* LINTED deliberate lack of effect */
	(void)k;
	
	return (0);
}

static pthread_t
_pthread_self(void)
{

	return (0);
}

static int
_pthread_sigmask(int h, const sigset_t *s, sigset_t *o)
{
	/* LINTED deliberate lack of effect */
	(void)h;
	/* LINTED deliberate lack of effect */
	(void)s;
	/* LINTED deliberate lack of effect */
	(void)o;

	return (0);
}

static int *
_pthread__errno(void)
{

	return NULL;
}



static pthread_ops_t __null_pthread_ops = {
	_pthread_mutex_init,
	_pthread_mutex_lock,
	NULL, /* _pthread_mutex_trylock */
	_pthread_mutex_unlock,
	NULL, /* _pthread_mutex_destroy */
	NULL, /* _pthread_mutexattr_init */
	NULL, /* _pthread_mutexattr_destroy */

	_pthread_cond_init,
	_pthread_cond_signal,
	NULL, /* _pthread_cond_broadcast */
	_pthread_cond_wait,
	NULL, /* _pthread_cond_timedwait */
	NULL, /* _pthread_cond_destroy */
	NULL, /* _pthread_condattr_init */
	NULL, /* _pthread_condattr_destroy */

	_pthread_key_create,
	_pthread_setspecific,
	_pthread_getspecific,
	NULL, /* _pthread_key_delete */

	_pthread_once,

	_pthread_self,

	_pthread_sigmask,

	_pthread__errno
};

pthread_ops_t *__libc_pthread_ops = &__null_pthread_ops;
