/*	$NetBSD: mutex.h,v 1.3.2.1 2007/02/27 16:52:59 yamt Exp $	*/

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

#ifndef _SH3_MUTEX_H_
#define	_SH3_MUTEX_H_

/*
 * The SH3 mutex implementation is troublesome, because SH3 lacks
 * a pointer sized compare-and-set operation.  SMP for spin mutexes
 * is easy - we don't need to know who owns the lock.  For adaptive
 * mutexes, we need an owner field and additional interlock
 *
 * See arch/sparc/include/mutex.h for a more detailed description.
 */

#ifndef __MUTEX_PRIVATE

struct kmutex {
	uintptr_t	mtx_pad1;
	uint32_t	mtx_pad2;
};

#else	/* __MUTEX_PRIVATE */

struct kmutex {
	union {
		/* Adaptive mutex */
		volatile uintptr_t	mtxu_owner;		/* 0-3 */
		__cpu_simple_lock_t	mtxu_interlock;		/* 0 */

		/* Spin mutex. */
		struct {
			uint8_t			mtxs_dummy;	/* 0 */
			uint8_t			mtxs_unused1;	/* 1 */
			ipl_cookie_t		mtxs_ipl;	/* 2 */
			uint8_t			mtxs_unused2;	/* 3 */
		} s;
	} u;
	__cpu_simple_lock_t	mtx_lock;			/* 4 */
	uint8_t			mtx_idtype[3];			/* 5-7 */
};

#undef	__HAVE_MUTEX_STUBS

#define	mtx_owner	u.mtxu_owner
#define	mtx_interlock	u.mtxu_interlock
#define	mtx_dummy	u.s.mtxs_dummy
#define	mtx_ipl		u.s.mtxs_ipl

static inline uintptr_t
MUTEX_OWNER(uintptr_t owner)
{
	return owner << 6;
}

static inline int
MUTEX_OWNED(uintptr_t owner)
{
	return owner != 0;
}

static inline int
MUTEX_SET_WAITERS(kmutex_t *mtx, uintptr_t owner)
{
	(void)__cpu_simple_lock_try(&mtx->mtx_lock);
 	return mtx->mtx_owner != 0;
}

static inline int
MUTEX_HAS_WAITERS(volatile kmutex_t *mtx)
{
	if (mtx->mtx_owner == 0)
		return 0;
	return mtx->mtx_lock == __SIMPLELOCK_LOCKED;
}

static inline void
MUTEX_INITIALIZE_SPIN(kmutex_t *mtx, u_int id, int ipl)
{
	mtx->mtx_idtype[0] = (uint8_t)id;
	mtx->mtx_idtype[1] = (uint8_t)(id >> 8);
	mtx->mtx_idtype[2] = (uint8_t)((id >> 16) | 0x80);
	mtx->mtx_ipl = makeiplcookie(ipl);
	mtx->mtx_interlock = __SIMPLELOCK_LOCKED;
	__cpu_simple_lock_init(&mtx->mtx_lock);
}

static inline void
MUTEX_INITIALIZE_ADAPTIVE(kmutex_t *mtx, u_int id)
{
	mtx->mtx_idtype[0] = (uint8_t)id;
	mtx->mtx_idtype[1] = (uint8_t)(id >> 8);
	mtx->mtx_idtype[2] = (uint8_t)(id >> 16);
	__cpu_simple_lock_init(&mtx->mtx_lock);
}

static inline void
MUTEX_DESTROY(kmutex_t *mtx)
{
	mtx->mtx_owner = (uintptr_t)-1L;
	mtx->mtx_idtype[0] = 0xff;
	mtx->mtx_idtype[1] = 0xff;
	mtx->mtx_idtype[2] = 0xff;
}

static inline u_int
MUTEX_GETID(kmutex_t *mtx)
{
	return (u_int)mtx->mtx_idtype[0] |
	    ((u_int)mtx->mtx_idtype[1] << 8) |
	    (((u_int)mtx->mtx_idtype[2] & 0x7f) << 16);
}

static inline int
MUTEX_SPIN_P(volatile kmutex_t *mtx)
{
	return mtx->mtx_idtype[2] & 0x80;
}

static inline int
MUTEX_ADAPTIVE_P(volatile kmutex_t *mtx)
{
	return (mtx->mtx_idtype[2] & 0x80) == 0;
}

static inline int
MUTEX_ACQUIRE(kmutex_t *mtx, uintptr_t curthread)
{
	if (!__cpu_simple_lock_try(&mtx->mtx_interlock))
		return 0;
	mtx->mtx_owner = (curthread >> 6) | 0xfa000000;
	return 1;
}

static inline void
MUTEX_RELEASE(kmutex_t *mtx)
{
	mtx->mtx_owner = 0;
	__cpu_simple_unlock(&mtx->mtx_lock);
}

static inline void
MUTEX_CLEAR_WAITERS(kmutex_t *mtx)
{
	__cpu_simple_unlock(&mtx->mtx_lock);
}

#endif	/* __MUTEX_PRIVATE */

#endif /* _SH3_MUTEX_H_ */
