/* $NetBSD: lock.h,v 1.4 1999/12/03 01:11:34 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *
 * NOTE: We assume that SIMPLELOCK_UNLOCKED == 0, so we can simply
 * store `zero' to release a lock.
 */

#ifndef _ALPHA_LOCK_H_
#define	_ALPHA_LOCK_H_

static __inline void cpu_simple_lock_init __P((__volatile struct simplelock *))
	__attribute__((__unused__));
static __inline void cpu_simple_lock __P((__volatile struct simplelock *))
	__attribute__((__unused__));
static __inline int cpu_simple_lock_try __P((__volatile struct simplelock *))
	__attribute__((__unused__));
static __inline void cpu_simple_unlock __P((__volatile struct simplelock *))
	__attribute__((__unused__));

static __inline void
cpu_simple_lock_init(alp)
	__volatile struct simplelock *alp;
{

	__asm __volatile(
		"# BEGIN cpu_simple_lock_init\n"
		"	stl	$31, %0		\n"
		"	mb			\n"
		"	# END cpu_simple_lock_init"
		: "=m" (alp->lock_data));
}

static __inline void
cpu_simple_lock(alp)
	__volatile struct simplelock *alp;
{
	unsigned long t0;

	/*
	 * Note, if we detect that the lock is held when
	 * we do the initial load-locked, we spin using
	 * a non-locked load to save the coherency logic
	 * some work.
	 */

	__asm __volatile(
		"# BEGIN cpu_simple_lock\n"
		"1:	ldl_l	%0, %3		\n"
		"	bne	%0, 2f		\n"
		"	bis	$31, %2, %0	\n"
		"	stl_c	%0, %1		\n"
		"	beq	%0, 3f		\n"
		"	mb			\n"
		"	br	4f		\n"
		"2:	ldl	%0, %3		\n"
		"	beq	%0, 1b		\n"
		"	br	2b		\n"
		"3:	br	1b		\n"
		"4:				\n"
		"	# END cpu_simple_lock\n"
		: "=r" (t0), "=m" (alp->lock_data)
		: "i" (SIMPLELOCK_LOCKED), "1" (alp->lock_data));
}

static __inline int
cpu_simple_lock_try(alp)
	__volatile struct simplelock *alp;
{
	unsigned long t0, v0;

	__asm __volatile(
		"# BEGIN cpu_simple_lock_try\n"
		"1:	ldl_l	%0, %4		\n"
		"	bne	%0, 2f		\n"
		"	bis	$31, %3, %0	\n"
		"	stl_c	%0, %2		\n"
		"	beq	%0, 3f		\n"
		"	mb			\n"
		"	bis	$31, 1, %1	\n"
		"	br	4f		\n"
		"2:	bis	$31, $31, %1	\n"
		"	br	4f		\n"
		"3:	br	1b		\n"
		"4:				\n"
		"	# END cpu_simple_lock_try"
		: "=r" (t0), "=r" (v0), "=m" (alp->lock_data)
		: "i" (SIMPLELOCK_LOCKED), "2" (alp->lock_data));

	return (v0);
}

static __inline void
cpu_simple_unlock(alp)
	__volatile struct simplelock *alp;
{

	__asm __volatile(
		"# BEGIN cpu_simple_unlock\n"
		"	stl	$31, %0		\n"
		"	mb			\n"
		"	# END cpu_simple_unlock"
		: "=m" (alp->lock_data));
}

#endif /* _ALPHA_LOCK_H_ */
