/*	$NetBSD: mutexmi.h,v 1.1.2.1 2006/11/17 16:34:40 ad Exp $	*/

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

/*
 * Slow, bare bones lock constructs for architectures that do not provide
 * them.
 */

#ifndef _SYS_MUTEXMI_H_
#define	_SYS_MUTEXMI_H_

struct kmutex {
	volatile uintptr_t	mtx_storage;
	volatile uint8_t	mtx_id[3];
	volatile uint8_t	mtx_flags;
};

#ifdef __MUTEX_PRIVATE

#define	MUTEX_INITIALIZE_ADAPTIVE(mtx)	/* will be zeroed */

#define	MUTEX_INITIALIZE_SPIN(mtx, ipl)					\
do {									\
	(mtx)->mtx_flags = MUTEX_BIT_SPIN;				\
	(mtx)->mtx_storage = (ipl);					\
} while (/* CONSTCOND */ 0)

#define	MUTEX_BIT_SPIN			0x01
#define	MUTEX_BIT_WAITERS		0x02
#define	MUTEX_BIT_SPIN_LOCKED		0x02

#define	MUTEX_SPIN_UNLOCKED		0x01
#define	MUTEX_SPIN_LOCKED		0x03

#define	MUTEX_SPIN_P(mtx)		\
    (((mtx)->mtx_flags & MUTEX_BIT_SPIN) != 0)
#define	MUTEX_ADAPTIVE_P(mtx)		\
    (((mtx)->mtx_flags & MUTEX_BIT_SPIN) == 0)

static inline void
MUTEX_SETID(struct kmutex *mtx, u_int id)
{
	mtx->mtx_id[0] = (uint8_t)id;
	mtx->mtx_id[1] = (uint8_t)(id >> 8);
	mtx->mtx_id[2] = (uint8_t)(id >> 16);
}

static inline u_int
MUTEX_GETID(struct kmutex *mtx)
{
	return mtx->mtx_id[0] | (mtx->mtx_id[1] << 8) | (mtx->mtx_id[2] << 16);
}

/*
 * Spin mutex methods.
 */

#define	MUTEX_SPIN_HELD(mtx)		\
    ((mtx)->mtx_flags == MUTEX_SPIN_LOCKED)

static inline u_int
MUTEX_SPIN_ACQUIRE(struct kmutex *mtx)
{
	if (mtx->mtx_flags != MUTEX_SPIN_UNLOCKED)
		return 0;
	mtx->mtx_flags = MUTEX_SPIN_LOCKED;
	return 1;
}

static inline void
MUTEX_SPIN_RELEASE(struct kmutex *mtx)
{
	__insn_barrier();
	mtx->mtx_flags = MUTEX_SPIN_UNLOCKED;
}

#define	MUTEX_SPIN_SPLSAVE(mtx, s)					\
do {									\
	struct cpu_info *x__ci = curcpu();				\
	if ((x__ci->ci_mtx_count)-- == 0)				\
		x__ci->ci_mtx_oldspl = (s);				\
} while (/* CONSTCOND */ 0)

#define	MUTEX_SPIN_SPLRESTORE(mtx)					\
do {									\
	struct cpu_info *x__ci = curcpu();				\
	int s = x__ci->ci_mtx_oldspl;					\
	__insn_barrier();						\
	if (++(x__ci->ci_mtx_count) == 0)				\
		splx(s);						\
} while (/* CONSTCOND */ 0)

#define	MUTEX_SPIN_MINSPL(mtx)		((mtx)->mtx_storage)
#define	MUTEX_SPIN_OLDSPL(ci)		((ci)->ci_mtx_oldspl)

/*
 * Adaptive mutex methods.
 */        
#define	MUTEX_OWNER(mtx)		((mtx)->mtx_storage)
#define	MUTEX_HAS_WAITERS(mtx)		((mtx)->mtx_flags)

static inline int
MUTEX_SET_WAITERS(struct kmutex *mtx)
{
	int s = splsched();
	if (mtx->mtx_storage == 0) {
		splx(s);
		return 0;
	}
	mtx->mtx_flags = MUTEX_BIT_WAITERS;
	splx(s);
	return 1;
}

static inline int
MUTEX_ACQUIRE(struct kmutex *mtx, uintptr_t thread)
{
	int s = splsched();
	if (mtx->mtx_storage != 0) {
		splx(s);
		return 0;
	}
	mtx->mtx_storage = thread;
	mtx->mtx_flags = 0;
	splx(s);
	return 1;
}

static inline void
MUTEX_RELEASE(kmutex_t *mtx)
{
	int s = splsched();
	mtx->mtx_storage = 0;
	splx(s);
}

#endif	/* __MUTEX_PRIVATE */

#endif /* _SYS_MUTEXMI_H_ */
