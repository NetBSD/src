/*	$NetBSD: mutex.h,v 1.1.2.7 2002/03/22 00:15:55 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *	* Spin -- These mutexes always spin, and are thus safe to use
 *	  when there is no valid thread context.  These are similar to
 *	  the Mach `simplelock'.  They also block interrupts as specified
 *	  at mutex init time.
 *
 * When adaptive mutexes are held, they are owned by threads.  When
 * Spin mutexes are held, they are owned by CPUs.
 *
 * Machine dependent code must provide the following:
 *
 *	struct mutex
 *		The actual mutex structure.  This structure is mostly
 *		opaque to machine-independent code; most access are done
 *		through macros.  However, machine-independent code must
 *		be able to access the following members:
 *
 *			__cpu_simple_lock_t m_spinlock
 *			int m_oldspl
 *			int m_minspl
 *
 *	MUTEX_INITIALIZER_ADAPTIVE
 *		Static initializer for adaptive mutexes.
 *
 *	MUTEX_INITIALIZER_SPIN(ipl)
 *		Static initializer for spin mutexes.
 *
 *	MUTEX_INIT(mtx, type)
 *		Initialize the machine-dependent mutex fields.
 *
 *	MUTEX_ADAPTIVE_P(mtx)
 *		Evaluates to true if the mutex is an adaptive mutex.
 *
 *	MUTEX_SPIN_P(mtx)
 *		Evaluates to true if the mutex is a spin mutex.
 *
 *	MUTEX_OWNER(mtx)
 *		Returns the owner of the adaptive mutex (struct proc *).
 *
 *	MUTEX_HAS_WAITERS(mtx)
 *		Returns true if the mutex has waiters.
 *
 *	MUTEX_SET_WAITERS(mtx)
 *		Mark the mutex has having waiters.
 *
 *	MUTEX_RELEASE(mtx)
 *		Release the lock and clear the "has waiters" indication.
 *
 *	mutex_enter(kmutex_t *mtx)
 *		Try to acquire the mutex.  If it is held, call
 *		mutex_vector_enter().
 *
 *	mutex_tryenter(kmutex_t *mtx)
 *		Try to acquire the mutex.  If it is held, call
 *		mutex_vector_tryenter().
 *
 *	mutex_exit(kmutex_t *mtx)
 *		Try to release the mutex.  If the owner field is
 *		not the current thread, call mutex_vector_exit().
 */

#include <sys/simplelock.h>

typedef int kmutex_type_t;

#define	MUTEX_ADAPTIVE		0
#define	MUTEX_SPIN		1

/*
 * By default, we prefer adaptive mutexes.  Device drivers, however,
 * since they generally use interrupts, get spin mutexes.
 */
#define	MUTEX_DEFAULT		MUTEX_ADAPTIVE
#define	MUTEX_DRIVER		MUTEX_SPIN

struct mutex_debug_info {
	vaddr_t		mtx_locked;	/* PC where mutex was locked */
	vaddr_t		mtx_unlocked;	/* PC where mutex was unlocked */
};

typedef struct mutex kmutex_t;

#ifdef _KERNEL
void	mutex_init(kmutex_t *, kmutex_type_t, int);
void	mutex_destroy(kmutex_t *);

void	mutex_enter(kmutex_t *);
int	mutex_tryenter(kmutex_t *);
void	mutex_exit(kmutex_t *);

void	mutex_vector_enter(kmutex_t *);
int	mutex_vector_tryenter(kmutex_t *);
void	mutex_vector_exit(kmutex_t *);

struct proc *mutex_owner(kmutex_t *);
int	mutex_owned(kmutex_t *);
#endif /* _KERNEL */

#include <machine/mutex_impl.h>

#endif /* _SYS_MUTEX_H_ */
