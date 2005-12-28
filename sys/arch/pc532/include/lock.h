/*	$NetBSD: lock.h,v 1.8 2005/12/28 19:09:29 perry Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gregory McGarry.
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
 * Machine-dependent spin lock operations.
 */

#ifndef _PC532_LOCK_H_
#define	_PC532_LOCK_H_

static __inline void __cpu_simple_lock_init __P((__cpu_simple_lock_t *))
	__attribute__((__unused__));
static __inline void __cpu_simple_lock __P((__cpu_simple_lock_t *))
	__attribute__((__unused__));
static __inline int __cpu_simple_lock_try __P((__cpu_simple_lock_t *))
	__attribute__((__unused__));
static __inline void __cpu_simple_unlock __P((__cpu_simple_lock_t *))
	__attribute__((__unused__));

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{

	 __asm volatile(
		"1:	sbitid	%1, %0	\n"
		"	bfs	1b	\n"
		: "=m" (*alp)
		: "i" (__SIMPLELOCK_LOCKED));
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	int __rv = 1;

	__asm volatile(
		"	sbitid	%2, %0	\n"
		"	bfc	1f	\n"
		"	movqd	0, %1	\n"
		"1:			\n"
		: "=m" (*alp), "=m" (__rv)
		: "i" (__SIMPLELOCK_LOCKED));

	return (__rv);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
}

#endif /* _PC532_LOCK_H_ */
