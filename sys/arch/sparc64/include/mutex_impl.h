/*	$NetBSD: mutex_impl.h,v 1.1.2.2 2002/03/19 05:17:35 thorpej Exp $	*/

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

#ifndef _SPARC64_MUTEX_IMPL_H_
#define	_SPARC64_MUTEX_IMPL_H_

struct mutex {
	union {
		/* Adaptive mutex */
		struct {
			__volatile unsigned long mtx_owner;	/* 0-7 */
#ifndef __arch64__
			unsigned long mtx_rsvd0;
#endif
		} mtx_adapt;

		/* Spin mutex */
		struct {
			/*
			 * Note: we want the dummy byte to be
			 * the LSB of the owner, since the LSB
			 * of the address of the owning thread
			 * could never possibly be 0xff.
			 */
#ifdef __arch64__
			short mtx_minspl;			/* 0-1 */
			short mtx_oldspl;			/* 2-3 */
			char mtx_rsvd[2];			/* 4-5 */
			__cpu_simple_lock_t mtx_lock;		/* 6 */
			char mtx_dummy;				/* 7 */
#else
			char mtx_rsvd[2];			/* 0-1 */
			__cpu_simple_lock_t mtx_lock;		/* 2 */
			char mtx_dummy;				/* 3 */
			short mtx_minspl;			/* 4-5 */
			short mtx_oldspl;			/* 6-7 */
#endif
		} mtx_spin;
	} mtx_un;
};

#define	MUTEX_INITIALIZER_ADAPTIVE					\
	{ { .mtx_adapt = { .mtx_owner = 0 } } }

#define	MUTEX_INITIALIZER_SPIN(ipl)					\
	{ { .mtx_spin = { .mtx_dummy = 0xff,				\
			  .mtx_lock = __SIMPLELOCK_UNLOCKED,		\
	  		  .mtx_minspl = (ipl) } } }

#define	m_owner			mtx_un.mtx_adapt.mtx_owner
#define	m_spinlock		mtx_un.mtx_spin.mtx_lock
#define	m_oldspl		mtx_un.mtx_spin.mtx_oldspl
#define	m_minspl		mtx_un.mtx_spin.mtx_minspl

#define	MUTEX_WAITERS		0x02
#ifdef __arch64__
#define	MUTEX_THREAD		0xfffffffffffffff0UL
#else
#define	MUTEX_THREAD		0xfffffff0UL
#endif

#define	MUTEX_INIT(mtx, type)						\
do {									\
	switch ((type)) {						\
	case MUTEX_ADAPTIVE:						\
		(mtx)->m_owner = 0;					\
		break;							\
	case MUTEX_SPIN:						\
		(mtx)->mtx_un.mtx_spin.mtx_dummy = 0xff;		\
		__cpu_simple_lock_init(&(mtx)->m_spinlock);		\
		break;							\
	default:							\
		panic("MUTEX_INIT");					\
	}								\
	__asm __volatile("membar #MemIssue" ::: "memory");		\
} while (/*CONSTCOND*/0)

#define	MUTEX_ADAPTIVE_P(mtx)						\
	((mtx)->mtx_un.mtx_spin.mtx_dummy != 0xff)
#define	MUTEX_SPIN_P(mtx)						\
	((mtx)->mtx_un.mtx_spin.mtx_dummy == 0xff)

#define	MUTEX_OWNER(mtx)	((struct proc *)((mtx)->m_owner & MUTEX_THREAD))
#define	MUTEX_HAS_WAITERS(mtx)	(((mtx)->m_owner & MUTEX_WAITERS) != 0)

#define	MUTEX_SET_WAITERS(mtx)	_mutex_set_waiters((mtx))

#define	MUTEX_RELEASE(mtx)						\
do {									\
	__asm __volatile("membar #MemIssue" ::: "memory");		\
	(mtx)->m_owner = 0;						\
} while (/*CONSTCOND*/0)

#ifdef _KERNEL
void	_mutex_set_waiters(kmutex_t *);
#endif /* _KERNEL */

#endif /* _SPARC64_MUTEX_IMPL_H_ */
