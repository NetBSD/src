/*	$NetBSD: lock.h,v 1.9 2006/06/03 23:58:48 simonb Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles.
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
 * Machine-dependent spin lock operations for MIPS R4000 Processors.
 *
 * Note:  R3000 doesn't have any atomic update instructions
 */

#ifndef _MIPS_LOCK_H_
#define	_MIPS_LOCK_H_

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *lp)
{

	__asm volatile(
		"# -- BEGIN __cpu_simple_lock_init\n"
		"	.set push		\n"
		"	.set mips2		\n"
		"	sw	$0, %0		\n"
		"	sync			\n"
		"	.set pop		\n"
		"# -- END __cpu_simple_lock_init\n"
		: "=m" (*lp));
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *lp)
{
	unsigned long t0;

	/*
	 * Note, if we detect that the lock is held when
	 * we do the initial load-locked, we spin using
	 * a non-locked load to save the coherency logic
	 * some work.
	 */

	__asm volatile(
		"# -- BEGIN __cpu_simple_lock	\n"
		"	.set push		\n"
		"	.set mips2		\n"
		"1:	ll	%0, %3		\n"
		"	bnez	%0, 2f		\n"
		"	nop	       # BDslot	\n"
		"	li	%0, %2		\n"
		"	sc	%0, %1		\n"
		"	beqz	%0, 1b		\n"
		"	nop	       # BDslot	\n"
		"	nop			\n"
		"	sync			\n"
		"	j	3f		\n"
		"	nop			\n"
		"	nop			\n"
		"2:	lw	%0, %3		\n"
		"	bnez	%0, 2b		\n"
		"	nop	       # BDslot	\n"
		"	j	1b		\n"
		"	nop			\n"
		"3:				\n"
		"	.set pop		\n"
		"# -- END __cpu_simple_lock	\n"
		: "=r" (t0), "+m" (*lp)
		: "i" (__SIMPLELOCK_LOCKED), "m" (*lp));
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *lp)
{
	unsigned long t0, v0;

	__asm volatile(
		"# -- BEGIN __cpu_simple_lock_try\n"
		"	.set push		\n"
		"	.set mips2		\n"
		"1:	ll	%0, %4		\n"
		"	bnez	%0, 2f		\n"
		"	nop	       # BDslot	\n"
		"	li	%0, %3		\n"
		"	sc	%0, %2		\n"
		"	beqz	%0, 2f		\n"
		"	nop	       # BDslot	\n"
		"	li	%1, 1		\n"
		"	sync			\n"
		"	j	3f		\n"
		"	nop			\n"
		"	nop			\n"
		"2:	li	%1, 0		\n"
		"3:				\n"
		"	.set pop		\n"
		"# -- END __cpu_simple_lock_try	\n"
		: "=r" (t0), "=r" (v0), "+m" (*lp)
		: "i" (__SIMPLELOCK_LOCKED), "m" (*lp));

	return (v0 != 0);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *lp)
{

	__asm volatile(
		"# -- BEGIN __cpu_simple_unlock \n"
		"	.set push		\n"
		"	.set mips2		\n"
		"	sync			\n"
		"	sw	$0, %0		\n"
		"	.set pop		\n"
		"# -- END __cpu_simple_unlock	\n"
		: "=m" (*lp));
}
#endif /* _MIPS_LOCK_H_ */
