/* 	$NetBSD: lock.h,v 1.9 2005/12/28 19:09:29 perry Exp $	*/

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

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{
	__asm volatile(
		"	; BEGIN __cpu_simple_lock_init\n"
		"	stw	%1, %0		\n"
		"	sync			\n"
		"	; END __cpu_simple_lock_init"
		: "=m" (*alp)
		: "r" (__SIMPLELOCK_UNLOCKED));
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
	int32_t t0;

	/*
	 * Note, if we detect that the lock is held when
	 * we do the initial load-clear-word, we spin using
	 * a non-locked load to save the coherency logic
	 * some work.
	 */

#if 0
	__asm volatile(
		"	; BEGIN __cpu_simple_lock\n"
		"	ldcw		%1, %0		\n"
		"	comb,<>,n	%%r0,%0, 2f	\n"
		"1:	comb,=,n	%%r0,%0, 1b	\n"
		"	ldw		%1, %0		\n"
		"	ldcw		%1, %0		\n"
		"	comb,=,n	%%r0,%0, 1b	\n"
		"	ldw		%1, %0		\n"
		"2:	sync				\n"
		"	; END __cpu_simple_lock\n"
		: "=r" (t0), "+m" (*alp));
#else
	t0 = 1;
#endif
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	int32_t t0;

#if 0
	__asm volatile(
		"	; BEGIN __cpu_simple_lock_try\n"
		"	ldcw		%1, %0		\n"
		"	sync				\n"
		"	; END __cpu_simple_lock_try"
		: "=r" (t0), "+m" (*alp));
#else
	t0 = 1;
#endif
	return (t0 != 0);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{
	__asm volatile(
		"	; BEGIN __cpu_simple_unlock\n"
		"	sync			\n"
		"	stw	%1, %0		\n"
		"	; END __cpu_simple_unlock"
		: "+m" (*alp)
		: "r" (__SIMPLELOCK_UNLOCKED));
}

#endif /* _HPPA_LOCK_H_ */
