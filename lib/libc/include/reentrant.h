/*	$NetBSD: reentrant.h,v 1.8 2003/01/19 19:25:05 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by J.T. Conklin, by Nathan J. Williams, and by Jason R. Thorpe.
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
 * The thread primitives used by the library (mutex_t, mutex_lock, etc.)
 * are macros which expand to the cooresponding primitives provided by
 * the thread engine or to nothing.  The latter is used so that code is
 * not unreasonably cluttered with #ifdefs when all thread safe support
 * is removed.
 * 
 * The thread macros can be directly mapped to the mutex primitives from
 * pthreads, however it should be reasonably easy to wrap another mutex
 * implementation so it presents a similar interface.
 * 
 * The thread functions operate by dispatching to symbols which are, by
 * default, weak-aliased to no-op functions in thread-stub/thread-stub.c
 * (some uses of thread operations are conditional on __isthreaded, but
 * not all of them are).
 *
 * When the thread library is linked in, it provides strong-alias versions
 * of those symbols which dispatch to its own real thread operations.
 *
 * [This interface has been exposed to simplify making other libraries
 * thread-safe.]
 */

#ifdef _REENTRANT

#include <threadlib.h>

#define	FLOCKFILE(fp)		flockfile(fp)
#define	FUNLOCKFILE(fp)		funlockfile(fp)

#else /* _REENTRANT */

#define	mutex_init(m, a)
#define	mutex_lock(m)
#define	mutex_trylock(m)
#define	mutex_unlock(m)
#define	mutex_destroy(m)

#define	cond_init(c, t, a)
#define	cond_signal(c)
#define	cond_broadcast(c)
#define	cond_wait(c, m)
#define	cond_timedwait(c, m, t)
#define	cond_destroy(c)

#define	rwlock_init(l, a)
#define	rwlock_rdlock(l)
#define	rwlock_wrlock(l)
#define	rwlock_tryrdlock(l)
#define	rwlock_trywrlock(l)
#define	rwlock_unlock(l)
#define	rwlock_destroy(l)

#define	thr_keycreate(k, d)
#define	thr_setspecific(k, p)
#define	thr_getspecific(k)
#define	thr_keydelete(k)

#define	thr_once(o, f)
#define	thr_sigsetmask(f, n, o)
#define	thr_self()
#define	thr_errno()

#define	FLOCKFILE(fp)		
#define	FUNLOCKFILE(fp)		

#endif /* _REENTRANT */
