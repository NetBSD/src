/*	$NetBSD: mutex.h,v 1.2.4.4 2008/01/21 09:37:32 yamt Exp $	*/

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

#ifndef _MIPS_MUTEX_H_
#define	_MIPS_MUTEX_H_

#ifndef __MUTEX_PRIVATE

struct kmutex {
	uintptr_t	mtx_pad1;
	uint32_t	mtx_pad2[2];
};

#else	/* __MUTEX_PRIVATE */

#ifdef __MUTEX_PRIVATE
#include <machine/lock.h>
#endif

struct kmutex {
	volatile uintptr_t	mtx_owner;
	ipl_cookie_t		mtx_ipl;
	__cpu_simple_lock_t	mtx_lock;
};

#define	__HAVE_SIMPLE_MUTEXES		1
#define	__HAVE_MUTEX_STUBS		1
#define	__HAVE_SPIN_MUTEX_STUBS		1

#define	MUTEX_RECEIVE(mtx)		membar_enter()
#define	MUTEX_GIVE(mtx)			membar_exit()

#define	MUTEX_CAS(p, o, n)		\
    (_atomic_cas_ulong((volatile unsigned long *)(p), (o), (n)) == (o))

unsigned long	_atomic_cas_ulong(volatile unsigned long *,
    unsigned long, unsigned long);

#endif	/* __MUTEX_PRIVATE */

#endif /* _MIPS_MUTEX_H_ */

