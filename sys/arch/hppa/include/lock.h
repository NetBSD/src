/* 	$NetBSD: lock.h,v 1.10 2006/09/22 08:31:34 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and Matthew Fredette.
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

#ifndef _HPPA_LOCK_H_
#define	_HPPA_LOCK_H_

static __inline int
__ldcw(__cpu_simple_lock_t *__ptr)
{
	int __val;

	__asm volatile("ldcw 0(%1), %0"
	    : "=r" (__val) : "r" (__ptr)
	    : "memory");

	return __val;
}

static __inline void
__sync(void)
{

	__asm volatile("sync\n"
		: /* no outputs */
		: /* no inputs */
		: "memory");
}

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
	__sync();
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{

	/*
	 * Note, if we detect that the lock is held when
	 * we do the initial load-clear-word, we spin using
	 * a non-locked load to save the coherency logic
	 * some work.
	 */

	while (__ldcw(alp) == __SIMPLELOCK_LOCKED)
		while (*alp == __SIMPLELOCK_LOCKED)
			;
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{

	return (__ldcw(alp) != __SIMPLELOCK_LOCKED);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{
	__sync();
	*alp = __SIMPLELOCK_UNLOCKED;
}

#endif /* _HPPA_LOCK_H_ */
