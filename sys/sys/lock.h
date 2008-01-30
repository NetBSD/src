/*	$NetBSD: lock.h,v 1.82 2008/01/30 14:54:25 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Ross Harvey, and by Andrew Doran.
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
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code contains ideas from software contributed to Berkeley by
 * Avadis Tevanian, Jr., Michael Wayne Young, and the Mach Operating
 * System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)lock.h	8.12 (Berkeley) 5/19/95
 */

#ifndef	_SYS_LOCK_H_
#define	_SYS_LOCK_H_

#include <sys/stdint.h>
#include <sys/mutex.h>

#include <machine/lock.h>

#define	LK_TYPE_MASK	0x0000000f	/* type of lock sought */
#define	LK_SHARED	0x00000001	/* shared lock */
#define	LK_EXCLUSIVE	0x00000002	/* exclusive lock */
#define	LK_RELEASE	0x00000006	/* release any type of lock */
#define	LK_EXCLOTHER	0x00000008	/* other process holds lock */

#define	LK_NOWAIT	0x00000010	/* do not sleep to await lock */
#define	LK_CANRECURSE	0x00000040	/* this may be recursive lock attempt */
#define	LK_INTERLOCK	0x00010000	/* unlock passed simple lock after
					   getting lk_interlock */
#define	LK_RETRY	0x00020000	/* vn_lock: retry until locked */

#ifdef _KERNEL

/*
 * From <machine/lock.h>.
 */
#ifndef SPINLOCK_SPIN_HOOK
#define	SPINLOCK_SPIN_HOOK
#endif
#ifndef SPINLOCK_BACKOFF_HOOK
#define	SPINLOCK_BACKOFF_HOOK		nullop(NULL)
#endif
#ifndef	SPINLOCK_BACKOFF_MIN
#define	SPINLOCK_BACKOFF_MIN	4
#endif
#ifndef	SPINLOCK_BACKOFF_MAX
#define	SPINLOCK_BACKOFF_MAX	128
#endif

#define	SPINLOCK_BACKOFF(count)					\
do {								\
	int __i;						\
	for (__i = (count); __i != 0; __i--) {			\
		SPINLOCK_BACKOFF_HOOK;				\
	}							\
	if ((count) < SPINLOCK_BACKOFF_MAX)			\
		(count) += (count);				\
} while (/* CONSTCOND */ 0);

#define	SPINLOCK_RUN_HOOK(count)	((count) >= SPINLOCK_BACKOFF_MAX)

#ifdef LOCKDEBUG
#define	SPINLOCK_SPINOUT(spins)		((spins)++ > 0x0fffffff)
#else
#define	SPINLOCK_SPINOUT(spins)		((void)(spins), 0)
#endif

extern __cpu_simple_lock_t kernel_lock[];

#endif /* _KERNEL */

#endif /* _SYS_LOCK_H_ */
