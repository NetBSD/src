/*	$NetBSD: mutex.h,v 1.4.2.1 2007/05/08 22:28:40 skrll Exp $	*/

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

#ifndef _HPPA_MUTEX_H_
#define	_HPPA_MUTEX_H_

/*
 * The HPPA mutex implementation is troublesome, because HPPA lacks
 * a compare-and-set operation, yet there are many SMP HPPA machines
 * in circulation.  SMP for spin mutexes is easy - we don't need to
 * know who owns the lock.  For adaptive mutexes, we need an owner
 * field and additional interlock
 */

#ifndef __ASSEMBLER__

#include <machine/lock.h>

struct kmutex {
	union {
		/*
		 * Only the low 4 bytes of the lock will be used by
		 * __cpu_simple_lock(), but it must be aligned on a
		 * 16-byte boundary.  See hppa/lock.h
		 */
#ifdef __MUTEX_PRIVATE
		__cpu_simple_lock_t	mtxu_lock;		/* 0-15 */
		struct {
			volatile uint32_t	mtxs_lockword;	/* 0-3 */
			volatile uint32_t	mtxs_owner;	/* 4-7 */
			ipl_cookie_t		mtxs_ipl;	/* 8-11 */
			volatile uint8_t	mtxs_waiters;	/* 12 */

			/* For LOCKDEBUG */
			uint8_t			mtxs_id[3];	/* 13-15 */
		} s;
#endif
		uint8_t			mtxu_pad[16];		/* 0-15 */
	} u;
} __aligned (16);
#endif

#ifdef __MUTEX_PRIVATE

#define	__HAVE_MUTEX_STUBS	1

#define	mtx_lock	u.mtxu_lock
#define	mtx_owner	u.s.mtxs_owner
#define	mtx_ipl		u.s.mtxs_ipl
#define	mtx_waiters	u.s.mtxs_waiters
#define	mtx_id		u.s.mtxs_id

/* Magic constants for mtx_owner */
#define	MUTEX_ADAPTIVE_UNOWNED		0xffffff00
#define	MUTEX_SPIN_FLAG			0xffffff10
#define	MUTEX_UNOWNED_OR_SPIN(x)	(((x) & 0xffffffef) == 0xffffff00)

#ifndef __ASSEMBLER__

static inline uintptr_t
MUTEX_OWNER(uintptr_t owner)
{
	return owner;
}

static inline int
MUTEX_OWNED(uintptr_t owner)
{
	return owner != MUTEX_ADAPTIVE_UNOWNED;
}

static inline int
MUTEX_SET_WAITERS(kmutex_t *mtx, uintptr_t owner)
{
	mb_write();
	mtx->mtx_waiters = 1;
	mb_memory();
	return mtx->mtx_owner != MUTEX_ADAPTIVE_UNOWNED;
}

static inline int
MUTEX_HAS_WAITERS(volatile kmutex_t *mtx)
{
	return mtx->mtx_waiters != 0;
}

static inline void
MUTEX_INITIALIZE_SPIN(kmutex_t *mtx, u_int id, int ipl)
{
	mtx->mtx_ipl = makeiplcookie(ipl);
	mtx->mtx_id[0] = (uint8_t)id;
	mtx->mtx_id[1] = (uint8_t)(id >> 8);
	mtx->mtx_id[2] = (uint8_t)(id >> 16);
	mtx->mtx_owner = MUTEX_SPIN_FLAG;
	__cpu_simple_lock_init(&mtx->mtx_lock);
}

static inline void
MUTEX_INITIALIZE_ADAPTIVE(kmutex_t *mtx, u_int id)
{
	mtx->mtx_id[0] = (uint8_t)id;
	mtx->mtx_id[1] = (uint8_t)(id >> 8);
	mtx->mtx_id[2] = (uint8_t)(id >> 16);
	mtx->mtx_owner = MUTEX_ADAPTIVE_UNOWNED;
	__cpu_simple_lock_init(&mtx->mtx_lock);
}

static inline void
MUTEX_DESTROY(kmutex_t *mtx)
{
	mtx->mtx_owner = 0xffffffff;
	mtx->mtx_id[0] = 0xff;
	mtx->mtx_id[1] = 0xff;
	mtx->mtx_id[2] = 0xff;
}

static inline u_int
MUTEX_GETID(kmutex_t *mtx)
{
	return (u_int)mtx->mtx_id[0] |
	    ((u_int)mtx->mtx_id[1] << 8) |
	    ((u_int)mtx->mtx_id[2] << 16);
}

static inline int
MUTEX_SPIN_P(volatile kmutex_t *mtx)
{
	return mtx->mtx_owner == MUTEX_SPIN_FLAG;
}

static inline int
MUTEX_ADAPTIVE_P(volatile kmutex_t *mtx)
{
	return mtx->mtx_owner != MUTEX_SPIN_FLAG;
}

/* Acquire an adaptive mutex */
static inline int
MUTEX_ACQUIRE(kmutex_t *mtx, uintptr_t curthread)
{
	if (!__cpu_simple_lock_try(&mtx->mtx_lock))
		return 0;
	mtx->mtx_owner = curthread;
	return 1;
}

/* Release an adaptive mutex */
static inline void
MUTEX_RELEASE(kmutex_t *mtx)
{
	mtx->mtx_owner = MUTEX_ADAPTIVE_UNOWNED;
	__cpu_simple_unlock(&mtx->mtx_lock);
	mtx->mtx_waiters = 0;
}

static inline void
MUTEX_CLEAR_WAITERS(kmutex_t *mtx)
{
	mtx->mtx_waiters = 0;
}

#endif	/* __ASSEMBLER__ */

#endif	/* __MUTEX_PRIVATE */

#endif /* _HPPA_MUTEX_H_ */
