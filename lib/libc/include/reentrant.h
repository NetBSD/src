/*	$NetBSD: reentrant.h,v 1.6.4.5 2002/05/02 16:56:34 nathanw Exp $	*/

/*-
 * Copyright (c) 1997,98 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by J.T. Conklin.
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
 * Requirements:
 * 
 * 1. The thread safe mechanism should be lightweight so the library can
 *    be used by non-threaded applications without unreasonable overhead.
 * 
 * 2. There should be no dependency on a thread engine for non-threaded
 *    applications.
 * 
 * 3. There should be no dependency on any particular thread engine.
 * 
 * 4. The library should be able to be compiled without support for thread
 *    safety.
 * 
 * 
 * Rationale:
 * 
 * One approach for thread safety is to provide discrete versions of the
 * library: one thread safe, the other not.  The disadvantage of this is
 * that libc is rather large, and two copies of a library which are 99%+
 * identical is not an efficent use of resources.
 * 
 * Another approach is to provide a single thread safe library.  However,
 * it should not add significant run time or code size overhead to non-
 * threaded applications.
 * 
 * Since the NetBSD C library is used in other projects, it should be
 * easy to replace the mutual exclusion primitives with ones provided by
 * another system.  Similarly, it should also be easy to remove all
 * support for thread safety completely if the target environment does
 * not support threads.
 * 
 * 
 * Implementation Details:
 * 
 * The mutex primitives used by the library (mutex_t, mutex_lock, etc.)
 * are macros which expand to the cooresponding primitives provided by
 * the thread engine or to nothing.  The latter is used so that code is
 * not unreasonably cluttered with #ifdefs when all thread safe support
 * is removed.
 * 
 * The mutex macros can be directly mapped to the mutex primitives from
 * pthreads, however it should be reasonably easy to wrap another mutex
 * implementation so it presents a similar interface.
 * 
 * The mutex functions operate by dispatching through a vector of function
 * pointers; the pointer to this is defined in thread-stub/pthread-stub.c.
 * The pointer initially points to a set of no-op functions for non-threaded
 * use (most uses of thread operations are conditional on __isthreaded, but
 * a few aren't, so we have to have a vector of stub operations rather than
 * just leaving the vector pointer NULL).
 *
 * When a thread library is linked in, the library's initialization routine
 * will set the vector pointer to point to real thread operations.
 */

#ifdef _REENTRANT

#include <pthread.h>

#define mutex_t			pthread_mutex_t
#define MUTEX_INITIALIZER	PTHREAD_MUTEX_INITIALIZER

#define cond_t			pthread_cond_t
#define COND_INITIALIZER	PTHREAD_COND_INITIALIZER

#define rwlock_t		pthread_mutex_t
#define RWLOCK_INITIALIZER	PTHREAD_MUTEX_INITIALIZER

#define thread_key_t		pthread_key_t

#define thr_t			pthread_t

#define once_t			pthread_once_t
#define ONCE_INITIALIZER	PTHREAD_ONCE_INIT

extern pthread_ops_t *__libc_pthread_ops;

#define mutex_init(m, a)	((__libc_pthread_ops->mutex_init)((m),(a)))
#define mutex_lock(m)		((__libc_pthread_ops->mutex_lock)((m)))
#define mutex_unlock(m)		((__libc_pthread_ops->mutex_unlock)((m)))

#define cond_init(c, t, a)     	((__libc_pthread_ops->cond_init)((c), (a)))
#define cond_signal(m)		((__libc_pthread_ops->cond_signal)((m)))
#define cond_wait(c, m)		((__libc_pthread_ops->cond_wait)((c), (m)))

#define rwlock_init(l, a)	((__libc_pthread_ops->mutex_init)((l), (a)))
#define rwlock_rdlock(l)	((__libc_pthread_ops->mutex_lock)((l)))
#define rwlock_wrlock(l)	((__libc_pthread_ops->mutex_lock)((l)))
#define rwlock_unlock(l)	((__libc_pthread_ops->mutex_unlock)((l)))

#define thr_keycreate(k, d)	((__libc_pthread_ops->thr_keycreate)((k), (d)))
#define thr_setspecific(k, p)	((__libc_pthread_ops->thr_setspecific)((k), (p)))
#define thr_getspecific(k)	((__libc_pthread_ops->thr_getspecific)((k)))

#define thr_sigsetmask(f, n, o)	((__libc_pthread_ops->thr_sigsetmask)((f), (n), (o)))

#define thr_once(o,f)		((__libc_pthread_ops->thr_once)((o),(f)))

#define thr_self()		((__libc_pthread_ops->thr_self)())

#define thr_errno()		((__libc_pthread_ops->thr_errno)())

#define FLOCKFILE(fp)		flockfile(fp)
#define FUNLOCKFILE(fp)		funlockfile(fp)

#else

#define mutex_init(m, a)	
#define mutex_lock(m)		
#define mutex_unlock(m)		

#define cond_signal(m)
#define cond_wait(c, m)
#define cond_init(c, a, p)

#define rwlock_init(l, a)	
#define rwlock_rdlock(l)	
#define rwlock_wrlock(l)	
#define rwlock_unlock(l)	

#define thr_keycreate(k, d)
#define thr_setspecific(k, p)
#define thr_getspecific(k)

#define thr_sigsetmask(f, n, o)

#define thr_once(o, f)

#define thr_errno()

#define FLOCKFILE(fp)		
#define FUNLOCKFILE(fp)		

#endif
