/*	$NetBSD: mutex.h,v 1.1 2014/09/03 19:34:26 matt Exp $	*/

/*-
 * Copyright (c) 2002, 2007 The NetBSD Foundation, Inc.
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

#ifndef _OR1K_MUTEX_H_
#define	_OR1K_MUTEX_H_

#ifndef __MUTEX_PRIVATE

struct kmutex {
	uintptr_t	mtx_pad1;
};

#else	/* __MUTEX_PRIVATE */

struct kmutex {
	union {
		/* Adaptive mutex */
		volatile uintptr_t	mtxa_owner;

		/* Spin mutex */
		struct {
			/*
			 * Since the low bit of mtax_owner is used to flag this
			 * mutex as a spin mutex, we can't use the first byte
			 * or the last byte to store the ipl or lock values.
			 */
			volatile uint8_t	mtxs_dummy;
			ipl_cookie_t		mtxs_ipl;
			__cpu_simple_lock_t	mtxs_lock;
			volatile uint8_t	mtxs_unused;
		} s;
	} u;
};

#define	mtx_owner		u.mtxa_owner
#define	mtx_ipl			u.s.mtxs_ipl
#define	mtx_lock		u.s.mtxs_lock

#if 0
#define	__HAVE_MUTEX_STUBS		1
#define	__HAVE_SPIN_MUTEX_STUBS		1
#endif
#define	__HAVE_SIMPLE_MUTEXES		1

/*
 * MUTEX_RECEIVE: no memory barrier required; we're synchronizing against
 * interrupts, not multiple processors.
 */
#ifdef MULTIPROCESSOR
#define	MUTEX_RECEIVE(mtx)		membar_consumer()
#else
#define	MUTEX_RECEIVE(mtx)		/* nothing */
#endif

/*
 * MUTEX_GIVE: no memory barrier required; same reason.
 */
#ifdef MULTIPROCESSOR
#define	MUTEX_GIVE(mtx)			membar_producer()
#else
#define	MUTEX_GIVE(mtx)			/* nothing */
#endif

#define	MUTEX_CAS(p, o, n)		\
    (atomic_cas_ulong((volatile unsigned long *)(p), (o), (n)) == (o))

#endif	/* __MUTEX_PRIVATE */

#endif /* _OR1K_MUTEX_H_ */
