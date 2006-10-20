/*	$NetBSD: mutex.h,v 1.1.2.2 2006/10/20 19:28:11 ad Exp $	*/

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

#ifndef _X86_MUTEX_H_
#define	_X86_MUTEX_H_

/*
 * Spin mutexes take this format:
 *
 *	32     24       16      8          0
 *	+-----------------------------------+
 *	| oldspl | minspl | lock | 11111111 |
 *	+-----------------------------------+
 *
 * Adaptive mutexes take this format:
 *
 *	N             4     2          1   0
 *	+-----------------------------------+
 *	|        owner | resv | waiters | 0 |
 *	+-----------------------------------+
 */
struct kmutex {
	union {
		volatile uintptr_t	mtxa_owner;

		struct {
			uint8_t			mtxs_allone;
			volatile uint8_t	mtxs_lock;
			uint8_t			mtxs_minspl;
			uint8_t			mtxs_oldspl;
		} s;
	} u;
	uint32_t	mtx_id;
};

#ifdef __MUTEX_PRIVATE

#define	MUTEX_INITIALIZE_ADAPTIVE(mtx)	/* will be zeroed */

#define	MUTEX_INITIALIZE_SPIN(mtx, ipl)	\
do {					\
	(mtx)->mtx_allone = 0xff;	\
	(mtx)->mtx_minspl = (ipl);	\
} while (/* CONSTCOND */ 0)

#define	mtx_owner 	u.mtxa_owner
#define	mtx_allone 	u.s.mtxs_allone
#define	mtx_lock 	u.s.mtxs_lock
#define	mtx_minspl 	u.s.mtxs_minspl
#define	mtx_oldspl 	u.s.mtxs_oldspl

#define __HAVE_MUTEX_ENTER		1
#define __HAVE_MUTEX_EXIT		1

#define	MUTEX_BIT_WAITERS		0x01
#define	MUTEX_BIT_SPIN			0x02

#define	MUTEX_ADAPTIVE_P(mtx)		((mtx)->mtx_allone != 0xff)
#define	MUTEX_SPIN_P(mtx)		((mtx)->mtx_allone == 0xff)

#define	MUTEX_SETID(mtx, id)		((mtx)->mtx_id = id)
#define	MUTEX_GETID(mtx)		((mtx)->mtx_id)

/*
 * Spin mutex methods.
 */
static inline uint8_t
MUTEX_SPIN_HELD(kmutex_t *mtx)
{
	return mtx->mtx_lock;
}

static inline uint8_t
MUTEX_SPIN_ACQUIRE(kmutex_t *mtx)
{
	uint8_t rv;
	__asm volatile ("movb $1,%0; xchgb %0,%1; decb %0" :
			"=a" (rv) : "m" (mtx->mtx_lock) : "memory");
	return rv;
}

static inline void
MUTEX_SPIN_RELEASE(kmutex_t *mtx)
{
	__insn_barrier();
	mtx->mtx_lock = 0;
}

/*
 * Adaptive mutex methods.
 */        
#define	MUTEX_OWNER(mtx)						\
	((uintptr_t)((mtx)->mtx_owner & MUTEX_THREAD))
#define	MUTEX_HAS_WAITERS(mtx)						\
	(((int)(mtx)->mtx_owner & MUTEX_BIT_WAITERS) != 0)
#define	MUTEX_SET_WAITERS(mtx)						\
	_lock_set_waiters(&(mtx)->mtx_owner,				\
	     MUTEX_THREAD | MUTEX_BIT_SPIN, MUTEX_BIT_WAITERS)
#define	MUTEX_ACQUIRE(mtx, ct)						\
	_lock_cas(&(mtx)->mtx_owner, 0UL, (ct))

static inline void
MUTEX_RELEASE(kmutex_t *mtx)
{
	__insn_barrier();
	mtx->mtx_owner = 0;
}

#ifdef _KERNEL
int	_lock_cas(volatile uintptr_t *, uintptr_t, uintptr_t);
int	_lock_set_waiters(volatile uintptr_t *, uintptr_t, uintptr_t);
#endif /* _KERNEL */

#endif	/* __MUTEX_PRIVATE */

#endif /* _X86_MUTEX_H_ */
