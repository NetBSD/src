/*	$NetBSD: thread-stub.c,v 1.3 2003/01/19 19:48:45 thorpej Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Stubs for thread operations, for use when threads are not used by
 * the application.  See "reentrant.h" for details.
 */

#ifdef _REENTRANT

#define	__LIBC_THREAD_STUBS

#include "namespace.h"
#include "reentrant.h"

#include <signal.h>
#include <unistd.h>

extern int __isthreaded;

#define	DIE()	(void)kill(getpid(), SIGABRT)

#define	CHECK_NOT_THREADED_ALWAYS()	\
do {					\
	if (__isthreaded)		\
		DIE();			\
} while (/*CONSTCOND*/0)

#if 1
#define	CHECK_NOT_THREADED()	CHECK_NOT_THREADED_ALWAYS()
#else
#define	CHECK_NOT_THREADED()	/* nothing */
#endif

/* mutexes */

int	__libc_mutex_init_stub(mutex_t *, const mutexattr_t *);
int	__libc_mutex_catchall_stub(mutex_t *);

__weak_alias(__libc_mutex_init,__libc_mutex_init_stub)
__weak_alias(__libc_mutex_lock,__libc_mutex_catchall_stub)
__weak_alias(__libc_mutex_trylock,__libc_mutex_catchall_stub)
__weak_alias(__libc_mutex_unlock,__libc_mutex_catchall_stub)
__weak_alias(__libc_mutex_destroy,__libc_mutex_catchall_stub)

int
__libc_mutex_init_stub(mutex_t *m, const mutexattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)m;
	/* LINTED deliberate lack of effect */
	(void)a;

	CHECK_NOT_THREADED();

	return (0);
}

int
__libc_mutex_catchall_stub(mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)m;

	CHECK_NOT_THREADED();

	return (0);
}


/* condition variables */

int	__libc_cond_init_stub(cond_t *, const condattr_t *);
int	__libc_cond_wait_stub(cond_t *, mutex_t *);
int	__libc_cond_timedwait_stub(cond_t *, mutex_t *,
				   const struct timespec *);
int	__libc_cond_catchall_stub(cond_t *);

__weak_alias(__libc_cond_init,__libc_cond_init_stub)
__weak_alias(__libc_cond_signal,__libc_cond_catchall_stub)
__weak_alias(__libc_cond_broadcast,__libc_cond_catchall_stub)
__weak_alias(__libc_cond_wait,__libc_cond_wait_stub)
__weak_alias(__libc_cond_timedwait,__libc_cond_timedwait_stub)
__weak_alias(__libc_cond_destroy,__libc_cond_catchall_stub)

int
__libc_cond_init_stub(cond_t *c, const condattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)a;

	CHECK_NOT_THREADED();

	return (0);
}

int
__libc_cond_wait_stub(cond_t *c, mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)m;

	CHECK_NOT_THREADED();

	return (0);
}

int
__libc_cond_timedwait_stub(cond_t *c, mutex_t *m, const struct timespec *t)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)m;
	/* LINTED deliberate lack of effect */
	(void)t;

	CHECK_NOT_THREADED();

	return (0);
}

int
__libc_cond_catchall_stub(cond_t *c)
{
	/* LINTED deliberate lack of effect */
	(void)c;

	CHECK_NOT_THREADED();

	return (0);
}


/* read-write locks */

int	__libc_rwlock_init_stub(rwlock_t *, rwlockattr_t *);
int	__libc_rwlock_catchall_stub(rwlock_t *);

__weak_alias(__libc_rwlock_init,__libc_rwlock_init_stub)
__weak_alias(__libc_rwlock_rdlock,__libc_rwlock_catchall_stub)
__weak_alias(__libc_rwlock_wrlock,__libc_rwlock_catchall_stub)
__weak_alias(__libc_rwlock_tryrdlock,__libc_rwlock_catchall_stub)
__weak_alias(__libc_rwlock_trywrlock,__libc_rwlock_catchall_stub)
__weak_alias(__libc_rwlock_unlock,__libc_rwlock_catchall_stub)
__weak_alias(__libc_rwlock_destroy,__libc_rwlock_catchall_stub)

int
__libc_rwlock_init_stub(rwlock_t *l, rwlockattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)l;
	/* LINTED deliberate lack of effect */
	(void)a;

	CHECK_NOT_THREADED();

	return (0);
}

int
__libc_rwlock_catchall_stub(rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	CHECK_NOT_THREADED();

	return (0);
}


/* thread-specific data */

int	__libc_thr_keycreate_stub(thread_key_t *, void (*)(void *));
int	__libc_thr_setspecific_stub(thread_key_t, const void *);
void	*__libc_thr_getspecific_stub(thread_key_t);
int	__libc_thr_keydelete_stub(thread_key_t);

__weak_alias(__libc_thr_keycreate,__libc_thr_keycreate_stub)
__weak_alias(__libc_thr_setspecific,__libc_thr_setspecific_stub)
__weak_alias(__libc_thr_getspecific,__libc_thr_getspecific_stub)
__weak_alias(__libc_thr_keydelete,__libc_thr_keydelete_stub)

int
__libc_thr_keycreate_stub(thread_key_t *k, void (*d)(void *))
{
	/* LINTED deliberate lack of effect */
	(void)k;
	/* LINTED deliberate lack of effect */
	(void)d;

	CHECK_NOT_THREADED();

	return (0);
}

int
__libc_thr_setspecific_stub(thread_key_t k, const void *v)
{
	/* LINTED deliberate lack of effect */
	(void)k;
	/* LINTED deliberate lack of effect */
	(void)v;

	DIE();

	return (0);
}

void *
__libc_thr_getspecific_stub(thread_key_t k)
{
	/* LINTED deliberate lack of effect */
	(void)k;

	DIE();

	return (NULL);
}

int
__libc_thr_keydelete_stub(thread_key_t k)
{
	/* LINTED deliberate lack of effect */
	(void)k;

	CHECK_NOT_THREADED();

	return (0);
}


/* misc. */

int	__libc_thr_once_stub(once_t *, void (*)(void));
int	__libc_thr_sigsetmask_stub(int, const sigset_t *, sigset_t *);
thr_t	__libc_thr_self_stub(void);
int	*__libc_thr_errno_stub(void);

__weak_alias(__libc_thr_once,__libc_thr_once_stub)
__weak_alias(__libc_thr_sigsetmask,__libc_thr_sigsetmask_stub)
__weak_alias(__libc_thr_self,__libc_thr_self_stub)
__weak_alias(__libc_thr_errno,__libc_thr_errno_stub)

int
__libc_thr_once_stub(once_t *o, void (*r)(void))
{

	/* XXX Knowledge of libpthread types. */

	if (o->pto_done == 0) {
		(*r)();
		o->pto_done = 1;
	}

	return (0);
}

int
__libc_thr_sigsetmask_stub(int h, const sigset_t *s, sigset_t *o)
{
	/* LINTED deliberate lack of effect */
	(void)h;
	/* LINTED deliberate lack of effect */
	(void)s;
	/* LINTED deliberate lack of effect */
	(void)o;

	CHECK_NOT_THREADED();

	/* XXX just use sigmask(2)?  abort? */

	return (0);
}

thr_t
__libc_thr_self_stub(void)
{

	DIE();

	return (NULL);
}

int *
__libc_thr_errno_stub(void)
{

	DIE();

	return (NULL);
}

#endif /* _REENTRANT */
