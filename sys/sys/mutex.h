/*	$NetBSD: mutex.h,v 1.1.36.2 2006/10/20 19:45:12 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _SYS_MUTEX_H_
#define	_SYS_MUTEX_H_

/*
 * There are 2 types of mutexes:
 *
 *	* Adaptive -- If the lock is already held, the thread attempting
 *	  to acquire the lock determines if the thread that holds it is
 *	  currently running.  If so, it spins, else it sleeps.
 *
 *	* Spin -- If the lock is already held, the thead attempting to
 *	  acquire the lock spins.  The IPL will be raised on entry.
 *
 * Machine dependent code must provide the following:
 *
 *	struct mutex
 *		The actual mutex structure.  This structure is mostly
 *		opaque to machine-independent code; most access are done
 *		through macros.  However, machine-independent code must
 *		be able to access the following members:
 *
 *			integer m_oldspl
 *			integer m_minspl
 *
 *	MUTEX_INITIALIZER_SPIN(ipl)
 *		Static initializer for spin mutexes.
 *
 *	MUTEX_INITIALIZER_ADAPTIVE()
 *		Static initializer for adaptive mutexes.
 *
 *	MUTEX_ADAPTIVE_P(mtx)
 *		Evaluates to true if the mutex is an adaptive mutex.
 *
 *	MUTEX_SPIN_P(mtx)
 *		Evaluates to true if the mutex is a spin mutex.
 *
 *	MUTEX_OWNER(mtx)
 *		Returns the owner of the adaptive mutex (struct lwp *).
 *
 *	MUTEX_HAS_WAITERS(mtx)
 *		Returns true if the mutex has waiters.
 *
 *	MUTEX_SET_WAITERS(mtx)
 *		Mark the mutex has having waiters.
 *
 *	MUTEX_ACQUIRE(mtx, owner)
 *		Try to acquire an adaptive mutex such that:
 *			if (lock held OR waiters)
 *				return 0;
 *			else
 *				return 1;
 *		Must be MP/interrupt atomic.
 *
 *	MUTEX_RELEASE(mtx)
 *		Release the lock and clear the "has waiters" indication.
 *		Must be MP/interrupt atomic.
 *
 *	MUTEX_SPIN_ACQUIRE(mtx)
 *		Attempt to acquire a spin mutex.  Return non-zero on
 *		success.
 *
 *	MUTEX_SPIN_RELEASE(mtx)
 *		Release a spin mutex.
 * 
 *	MUTEX_SPIN_HELD(mtx)
 *		Evaluates to true if the spin mutex is held.
 *
 * Machine dependent code may optionally provide stubs for the following
 * functions to implement the easy (unlocked / no waiters) cases:
 *
 *	mutex_enter()
 *	mutex_exit(), mutex_exit_linked()
 *
 * For each function implemented, define the associated preprocessor
 * macro in the MD mutex header file.  For example: __HAVE_MUTEX_ENTER.
 * See kern_mutex.c for the MI stubs.
 */

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

typedef enum kmutex_type_t {
	MUTEX_SPIN = 0,
	MUTEX_ADAPTIVE = 1,
	MUTEX_DEFAULT = 2,
	MUTEX_DRIVER = 3
} kmutex_type_t;

typedef struct kmutex kmutex_t;

#include <machine/mutex.h>

#if defined(__MUTEX_PRIVATE) && defined(LOCKDEBUG)
#undef	__HAVE_MUTEX_ENTER
#undef	__HAVE_MUTEX_EXIT
#undef	__HAVE_MUTEX_EXIT_LINKED
#endif

#ifdef _KERNEL

void	mutex_init(kmutex_t *, kmutex_type_t, int);
void	mutex_destroy(kmutex_t *);

void	mutex_enter(kmutex_t *);
void	mutex_exit(kmutex_t *);

#if defined(__MUTEX_PRIVATE)

void	mutex_vector_enter(kmutex_t *, int);
void	mutex_vector_exit(kmutex_t *);

static inline int
mutex_getspl(kmutex_t *mtx)
{
	return mtx->mtx_oldspl;
}

static inline void
mutex_setspl(kmutex_t *mtx, int s)
{
	mtx->mtx_oldspl = s;
}
#endif	/* __MUTEX_PRIVATE */

void	mutex_enter(kmutex_t *);
void	mutex_exit(kmutex_t *);
int	mutex_tryenter(kmutex_t *);
void	mutex_exit_linked(kmutex_t *, kmutex_t *);

struct lwp *mutex_owner(kmutex_t *);
int	mutex_owned(kmutex_t *);

#endif /* _KERNEL */

#endif /* _SYS_MUTEX_H_ */
