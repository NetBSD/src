/*	$NetBSD: mutex_impl.h,v 1.1.2.3 2002/03/14 17:15:26 thorpej Exp $	*/

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

#ifndef _ALPHA_MUTEX_IMPL_H_
#define	_ALPHA_MUTEX_IMPL_H_

#include <machine/atomic.h>

struct mutex {
	union {
		/* Adaptive mutex */
		__volatile unsigned long mtx_owner;

		/* Spin mutex */
		struct {
			/*
			 * We're little-endian, so mtx_dummy is
			 * in the bottom-half of mtx_owner.
			 */
			unsigned int mtx_dummy;
			__cpu_simple_lock_t mtx_lock;
			int mtx_oldspl;
			int mtx_minspl;
		} mtx_spin;
	} mtx_un;
};

#define	MUTEX_INITIALIZER_ADAPTIVE					\
	{ { .mtx_owner = 0 } }

#define	MUTEX_INITIALIZER_SPIN(ipl)					\
	{ { .mtx_spin = { .mtx_dummy = MUTEX_SPIN,			\
			  .mtx_lock = __SIMPLELOCK_UNLOCKED,		\
	  		  .mtx_minspl = (ipl) } } }

#define	m_owner			mtx_un.mtx_owner
#define	m_spinlock		mtx_un.mtx_spin.mtx_lock
#define	m_oldspl		mtx_un.mtx_spin.mtx_oldspl
#define	m_minspl		mtx_un.mtx_spin.mtx_minspl

#define	MUTEX_WAITERS		0x02
#define	MUTEX_THREAD		0xfffffffffffffff0UL

#define	MUTEX_INIT(mtx, type)						\
do {									\
	switch ((type)) {						\
	case MUTEX_ADAPTIVE:						\
		(mtx)->m_owner = 0;					\
		break;							\
	case MUTEX_SPIN:						\
		(mtx)->mtx_un.mtx_spin.mtx_dummy = MUTEX_SPIN;		\
		__cpu_simple_lock_init(&(mtx)->m_spinlock);		\
		break;							\
	default:							\
		panic("MUTEX_INIT");					\
	}								\
	alpha_mb();							\
} while (/*CONSTCOND*/0)

#define	MUTEX_ADAPTIVE_P(mtx)						\
	(((mtx)->mtx_un.mtx_spin.mtx_dummy & MUTEX_SPIN) == 0)
#define	MUTEX_SPIN_P(mtx)						\
	(((mtx)->mtx_un.mtx_spin.mtx_dummy & MUTEX_SPIN) != 0)

#define	MUTEX_OWNER(mtx)	((struct proc *)((mtx)->m_owner & MUTEX_THREAD))
#define	MUTEX_HAS_WAITERS(mtx)	(((mtx)->m_owner & MUTEX_WAITERS) != 0)

#define	MUTEX_SET_WAITERS(mtx)						\
do {									\
	unsigned long _tmp_;						\
									\
	__asm __volatile(						\
		"# BEGIN MUTEX_SET_WAITERS\n"				\
		"1:	ldq_l	%0, %3		\n"			\
		"	beq	%0, 3f		\n"			\
		"	or	%0, %2, %0	\n"			\
		"	stq_c	%0, %1		\n"			\
		"	beq	%0, 2f		\n"			\
		"	mb			\n"			\
		"	br	3f		\n"			\
		"2:	br	1b		\n"			\
		"3:				\n"			\
		"	# END MUTEX_SET_WAITERS"			\
		: "=&r" (_tmp_), "=m" ((mtx)->m_owner)			\
		: "i" (MUTEX_WAITERS), "m" ((mtx)->m_owner)		\
		: "memory");						\
} while (/*CONSTCOND*/0)

#define	MUTEX_RELEASE(mtx)						\
do {									\
	alpha_mb();							\
	(mtx)->m_owner = 0;						\
} while (/*CONSTCOND*/0)

#endif /* _ALPHA_MUTEX_IMPL_H_ */
