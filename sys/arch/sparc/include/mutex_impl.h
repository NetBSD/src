/*	$NetBSD: mutex_impl.h,v 1.1.2.2 2002/03/19 05:14:57 thorpej Exp $	*/

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

#ifndef _SPARC_MUTEX_IMPL_H_
#define	_SPARC_MUTEX_IMPL_H_

struct mutex {
	union {
		/* Adaptive mutex */
		struct {
			__volatile unsigned long mtx_owner;	/* 0-3 */
			char mtx_waiters;			/* 4 */
			char mtx_rsvd[2];			/* 5-6 */
			char mtx_type;				/* 7 */
		} mtx_adapt;

		/* Spin mutex */
		struct {
			/*
			 * The dummy byte must overlap with the
			 * MSB of mtx_owner, so as to make the asm
			 * stubs think that the lock is always held.
			 */
			char mtx_dummy;				/* 0 */
			__cpu_simple_lock_t mtx_lock;		/* 1 */
			short mtx_minspl;			/* 2-3 */
			short mtx_oldspl;			/* 4-5 */
			char mtx_rsvd;				/* 6 */
			char mtx_type;				/* 7 */
		} mtx_spin;
	} mtx_un;
};

#define	MUTEX_INITIALIZER_ADAPTIVE					\
	{ { .mtx_adapt = { .mtx_owner = 0,				\
			   .mtx_waiters = 0,				\
			   .mtx_type = MUTEX_ADAPTIVE } } }

#define	MUTEX_INITIALIZER_SPIN(ipl)					\
	{ { .mtx_spin = { .mtx_dummy = 0xff,				\
			  .mtx_lock = __SIMPLELOCK_UNLOCKED,		\
			  .mtx_minspl = (ipl),				\
			  .mtx_type = MUTEX_SPIN } } }

#define	m_owner			mtx_un.mtx_adapt.mtx_owner
#define	m_spinlock		mtx_un.mtx_spin.mtx_lock
#define	m_oldspl		mtx_un.mtx_spin.mtx_oldspl
#define	m_minspl		mtx_un.mtx_spin.mtx_minspl

#define	MUTEX_THREAD		0x0ffffffffUL

#define	MUTEX_INIT(mtx, type)						\
do {									\
	switch ((type)) {						\
	case MUTEX_ADAPTIVE:						\
		(mtx)->m_owner = 0;					\
		(mtx)->mtx_un.mtx_adapt.mtx_waiters = 0;		\
		(mtx)->mtx_un.mtx_adapt.mtx_type = MUTEX_ADAPTIVE;	\
		break;							\
	case MUTEX_SPIN:						\
		(mtx)->mtx_un.mtx_spin.mtx_dummy = 0xff;		\
		__cpu_simple_lock_init(&(mtx)->m_spinlock);		\
		(mtx)->mtx_un.mtx_spin.mtx_type = MUTEX_SPIN;		\
		break;							\
	default:							\
		panic("MUTEX_INIT");					\
	}								\
} while (/*CONSTCOND*/0)

#define	MUTEX_ADAPTIVE_P(mtx)						\
	((mtx)->mtx_un.mtx_spin.mtx_type == MUTEX_ADAPTIVE)
#define	MUTEX_SPIN_P(mtx)						\
	((mtx)->mtx_un.mtx_spin.mtx_type == MUTEX_SPIN)

static __inline struct proc * __attribute__((__unused__))
MUTEX_OWNER(kmutex_t *mtx)
{
	unsigned long owner = mtx->m_owner;

	/*
	 * We need to handle the case where the lock is held but the
	 * owner hasn't stashed its thread pointer yet.
	 */
	return ((owner & 0x00ffffff) ?
	    ((struct proc *)((owner & MUTEX_THREAD) << 4)) : NULL);
}

#define	MUTEX_HAS_WAITERS(mtx)						\
	((mtx)->mtx_un.mtx_adapt.mtx_waiters != 0)

#define	MUTEX_SET_WAITERS(mtx)						\
do {									\
	(mtx)->mtx_un.mtx_adapt.mtx_waiters = 1;			\
} while (/*CONSTCOND*/0)

#define	MUTEX_RELEASE(mtx)						\
do {									\
	(mtx)->mtx_un.mtx_adapt.mtx_waiters = 0;			\
	(mtx)->m_owner = 0;						\
} while (/*CONSTCOND*/0)

#endif /* _SPARC_MUTEX_IMPL_H_ */
