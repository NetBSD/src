/*	$NetBSD: pthread-stub.c,v 1.3.6.8 2002/02/23 23:06:36 gmcgarry Exp $	*/

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

/*
 * When you update this file, don't forget to update pthread-intr.c
 * for those poor folks still using a.out.
 */

#ifdef __weak_alias
__weak_alias(pthread_mutex_init, _pthread_mutex_init)
__weak_alias(pthread_mutex_lock, _pthread_mutex_lock)
__weak_alias(pthread_mutex_unlock, _pthread_mutex_unlock)
#if 0
__weak_alias(pthread_rwlock_init, _pthread_rwlock_init)
__weak_alias(pthread_rwlock_rdlock, _pthread_rwlock_rdlock)
__weak_alias(pthread_rwlock_wrlock, _pthread_rwlock_wrlock)
__weak_alias(pthread_rwlock_unlock, _pthread_rwlock_unlock)
#endif
__weak_alias(pthread_once, _pthread_once)
__weak_alias(pthread_cond_init, _pthread_cond_init)
__weak_alias(pthread_cond_wait, _pthread_cond_wait)
__weak_alias(pthread_cond_signal, _pthread_cond_signal)
__weak_alias(pthread_spinlock, _pthread_spinlock)
__weak_alias(pthread_spinunlock, _pthread_spinunlock)
__weak_alias(pthread_key_create, _pthread_key_create)
__weak_alias(pthread_setspecific, _pthread_setspecific)
__weak_alias(pthread_getspecific, _pthread_getspecific)
__weak_alias(pthread_self, _pthread_self)
__weak_alias(pthread_sigmask, _pthread_sigmask)
__weak_alias(pthread__errno, _pthread__errno)
#endif

int _pthread_mutex_init(pthread_mutex_t *, pthread_mutexattr_t *);
int _pthread_mutex_lock(pthread_mutex_t *);
int _pthread_mutex_unlock(pthread_mutex_t *);
#if 0
int _pthread_rwlock_init(pthread_rwlock_t *, pthread_rwlockattr_t *);
int _pthread_rwlock_rdlock(pthread_rwlock_t *);
int _pthread_rwlock_wrlock(pthread_rwlock_t *);
int _pthread_rwlock_unlock(pthread_rwlock_t *);
#endif
int _pthread_once(pthread_once_t *, void (*)(void));
int _pthread_cond_init(pthread_cond_t *,const pthread_condattr_t *);
int _pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int _pthread_cond_signal(pthread_cond_t *);
void _pthread_spinlock(pthread_t, pthread_spin_t *);
void _pthread_spinunlock(pthread_t, pthread_spin_t *);
int _pthread_key_create(pthread_key_t *, void (*)(void *));
int _pthread_setspecific(pthread_key_t, const void *);
void* _pthread_getspecific(pthread_key_t);
pthread_t _pthread_self(void);
int _pthread_sigmask(int, const sigset_t *, sigset_t *);
int* _pthread__errno(void);


int
_pthread_mutex_init(pthread_mutex_t *m, pthread_mutexattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)m;
	/* LINTED deliberate lack of effect */
	(void)a;

	return (0);
}

int
_pthread_mutex_lock(pthread_mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)m;

	return (0);
}

int
_pthread_mutex_unlock(pthread_mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)m;

	return (0);
}
#if 0
int
_pthread_rwlock_init(pthread_rwlock_t *l, pthread_rwlockattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)l;
	/* LINTED deliberate lack of effect */
	(void)a;

	return (0);
}

int
_pthread_rwlock_rdlock(pthread_rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	return (0);
}

int
_pthread_rwlock_wrlock(pthread_rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	return (0);
}

int
_pthread_rwlock_unlock(pthread_rwlock_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)l;

	return (0);
}
#endif

int
_pthread_once(pthread_once_t *o, void (*r)(void))
{
	/* LINTED deliberate lack of effect */
	(void)o;
	/* LINTED deliberate lack of effect */
	(void)r;

	return (0);
}

int
_pthread_cond_init(pthread_cond_t *c,const pthread_condattr_t *a)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)a;

	return (0);
}

int
_pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	/* LINTED deliberate lack of effect */
	(void)m;

	return (0);
}

int
_pthread_cond_signal(pthread_cond_t *c)
{
	/* LINTED deliberate lack of effect */
	(void)c;
	
	return (0);
}

void 
_pthread_spinlock(pthread_t t, pthread_spin_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)t;
	/* LINTED deliberate lack of effect */
	(void)l;
}

void
_pthread_spinunlock(pthread_t t, pthread_spin_t *l)
{
	/* LINTED deliberate lack of effect */
	(void)t;
	/* LINTED deliberate lack of effect */
	(void)l;
}

int
_pthread_key_create(pthread_key_t *k, void (*d)(void *))
{
	/* LINTED deliberate lack of effect */
	(void)k;
	/* LINTED deliberate lack of effect */
	(void)d;

	return (0);
}

int
_pthread_setspecific(pthread_key_t k, const void *v)
{
	/* LINTED deliberate lack of effect */
	(void)k;
	/* LINTED deliberate lack of effect */
	(void)v;

	return (0);
}

void*
_pthread_getspecific(pthread_key_t k)
{
	/* LINTED deliberate lack of effect */
	(void)k;
	
	return (0);
}

pthread_t
_pthread_self(void)
{

	return (0);
}

int
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

int *
_pthread__errno(void)
{

	return NULL;
}
