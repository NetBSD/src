/*	$NetBSD: mutex.h,v 1.3.2.2 2007/02/27 16:53:21 yamt Exp $	*/

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

#ifndef _VAX_MUTEX_H_
#define	_VAX_MUTEX_H_

/*
 * The VAX mutex implementation is troublesome, because the VAX architecture
 * lacks a compare-and-set operation, yet there are many SMP VAX
 * machines in circulation.  SMP for spin mutexes is easy - we don't need
 * to know who owns the lock.  For adaptive mutexes, we need an aditional
 * interlock.  However, since we know that owners will be kernel addresses
 * and all kernel addresses have the high bit set, we can use the high bit
 * as an interlock.
 *
 * So we test the high bit with BBSSI and if clear
 * kernels are always loaded above 0xe0000000, and the low 5 bits of any
 * "struct lwp *" are always zero.  So, to record the lock owner, we only
 * need 23 bits of space.  mtxa_owner contains the mutex owner's address
 * shifted right by 5: the top three bits of which will always be 0xe,
 * overlapping with the interlock at the top byte, which is always 0xff
 * when the mutex is held.
 *
 * For a mutex acquisition, the owner field is set in two steps: first,
 * acquire the interlock (top bit), and second OR in the owner's address. 
 * Once the owner field is non zero, it will appear that the mutex is held,
 * by which LWP it does not matter: other LWPs competing for the lock will
 * fall through to mutex_vector_enter(), and either spin or sleep.
 *
 * As a result there is no space for a waiters bit in the owner field.  No
 * problem, because it would be hard to synchronise using one without a CAS
 * operation.  Note that in order to do unlocked release of adaptive
 * mutexes, we need the effect of MUTEX_SET_WAITERS() to be immediatley
 * visible on the bus.  So, adaptive mutexes share the spin lock byte with
 * spin mutexes (set with bb{cc,ss}i), but it is not treated as a lock in its
 * own right, rather as a flag that can be atomically set or cleared.
 *
 * When releasing an adaptive mutex, we first clear the owners field, and
 * then check to see if the waiters byte is set.  This ensures that there
 * will always be someone to wake any sleeping waiters up (even it the mutex
 * is acquired immediately after we release it, or if we are preempted
 * immediatley after clearing the owners field).  The setting or clearing of
 * the waiters byte is serialized by the turnstile chain lock associated
 * with the mutex.
 *
 * See comments in kern_mutex.c about releasing adaptive mutexes without
 * an interlocking step.
 */

#ifndef __MUTEX_PRIVATE

struct kmutex {
	uintptr_t	mtx_pad1;
	uint32_t	mtx_pad2[2];
};

#else	/* __MUTEX_PRIVATE */

struct kmutex {
	/* Adaptive mutex */
	union {
		volatile uintptr_t	mtxu_owner;	/* 0-3 */
		__cpu_simple_lock_t	mtxu_lock;	/* 0 */
	} mtx_u;
	ipl_cookie_t			mtx_ipl;	/* 4-7 */
	uint32_t			mtx_id;		/* 8-11 */
};
#define	mtx_owner	mtx_u.mtxu_owner
#define	mtx_lock	mtx_u.mtxu_lock

#define	__HAVE_MUTEX_STUBS		1
#define	__HAVE_SPIN_MUTEX_STUBS		1

#define	MUTEX_NO_SPIN_ACTIVE_P(ci)	((ci)->ci_mtx_count == 1)

static inline uintptr_t
MUTEX_OWNER(uintptr_t owner)
{
	return owner & ~1;
}

static inline bool
MUTEX_OWNED(uintptr_t owner)
{
	return owner != 0;
}

static inline bool
MUTEX_SET_WAITERS(kmutex_t *mtx, uintptr_t owner)
{
	mtx->mtx_owner |= 1;
 	return (mtx->mtx_owner & ~1) != 0;
}

static inline bool
MUTEX_HAS_WAITERS(volatile kmutex_t *mtx)
{
	return (mtx->mtx_owner & 1) != 0;
}

static inline void
MUTEX_CLEAR_WAITERS(volatile kmutex_t *mtx)
{
	mtx->mtx_owner &= ~1;
}

static inline void
MUTEX_INITIALIZE_SPIN(kmutex_t *mtx, u_int id, int ipl)
{
	mtx->mtx_id = (id << 1) | 1;
	mtx->mtx_ipl = makeiplcookie(ipl);
	mtx->mtx_lock = 0;
}

static inline void
MUTEX_INITIALIZE_ADAPTIVE(kmutex_t *mtx, u_int id)
{
	mtx->mtx_id = id << 1;
	mtx->mtx_ipl = makeiplcookie(-1);
	mtx->mtx_owner = 0;
}

static inline void
MUTEX_DESTROY(kmutex_t *mtx)
{
	mtx->mtx_owner = (uintptr_t)-1L;
	mtx->mtx_id = 0xdeadface << 1;
}

static inline u_int
MUTEX_GETID(volatile kmutex_t *mtx)
{
	return (mtx)->mtx_id >> 1;
}

static inline bool
MUTEX_SPIN_P(volatile kmutex_t *mtx)
{
	return (mtx->mtx_id & 1) != 0;
}

static inline bool
MUTEX_ADAPTIVE_P(volatile kmutex_t *mtx)
{
	return (mtx->mtx_id & 1) == 0;
}

static inline bool
MUTEX_ACQUIRE(kmutex_t *mtx, uintptr_t curthread)
{
	int rv;
	__asm __volatile(
		"clrl %1;"
		"bbssi $31,%0,1f;"
		"incl %1;"
		"insv %2,%0,$31,%0;"
		"1:"
	    : "=m"(mtx->mtx_owner), "=r"(rv)
	    : "g"(curthread));
	return rv;
}

static inline void
MUTEX_RELEASE(kmutex_t *mtx)
{
	__asm __volatile(
		"insv $0,$0,$31,%0;"
		"bbcci $31,%0,1f;"
		"1:"
	   : "=m" (mtx->mtx_owner));
}

#endif	/* __MUTEX_PRIVATE */

#endif /* _VAX_MUTEX_H_ */
