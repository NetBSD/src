/*	$NetBSD: rwlock_impl.h,v 1.1.2.2 2002/03/17 06:33:15 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _ALPHA_RWLOCK_IMPL_H_
#define	_ALPHA_RWLOCK_IMPL_H_

struct rwlock {
	__volatile unsigned long rwl_owner;
};

#define	RWLOCK_INITIALIZER						\
	{ .rwl_owner = 0 }

#define	RWLOCK_THREAD		0xfffffffffffffff0UL

#define	RWLOCK_INIT(rwl)						\
do {									\
	(rwl)->rwl_owner = 0;						\
	alpha_mb();							\
} while (/*CONSTCOND*/0)

#define	RWLOCK_ACQUIRE(rwl, old, new, actual)				\
do {									\
	unsigned long _tmp_;						\
									\
	__asm __volatile(						\
		"# BEGIN RWLOCK_ACQUIRE\n"				\
		"1:	ldq_l	%2, %5		\n"			\
		"	cmpeq	%2, %3, %0	\n"			\
		"	beq	%0, 3f		\n"			\
		"	mov	%4, %0		\n"			\
		"	stq_c	%0, %1		\n"			\
		"	beq	%0, 2f		\n"			\
		"	mb			\n"			\
		"	br	3f		\n"			\
		"2:	br	1b		\n"			\
		"3:				\n"			\
		"	# END RWLOCK_ACQUIRE"				\
		: "=&r" (_tmp_), "=m" ((rwl)->rwl_owner), "=&r" (actual)\
		: "r" (old), "r" (new), "m" ((rwl)->rwl_owner)		\
		: "memory");						\
} while (/*CONSTCOND*/0)

#define	RWLOCK_RELEASE(rwl, old, new, actual)				\
do {									\
	unsigned long _tmp_;						\
									\
	__asm __volatile(						\
		"# BEGIN RWLOCK_RELEASE\n"				\
		"1:	mb			\n"			\
		"	ldq_l	%2, %5		\n"			\
		"	cmpeq	%2, %3, %0	\n"			\
		"	beq	%0, 3f		\n"			\
		"	mov	%4, %0		\n"			\
		"	stq_c	%0, %1		\n"			\
		"	beq	%0, 2f		\n"			\
		"	br	3f		\n"			\
		"2:	br	1b		\n"			\
		"3:				\n"			\
		"	# END RWLOCK_RELEASE"				\
		: "=&r" (_tmp_), "=m" ((rwl)->rwl_owner), "=&r" (actual)\
		: "r" (old), "r" (new), "m" ((rwl)->rwl_owner)		\
		: "memory");						\
} while (/*CONSTCOND*/0)

#define	RWLOCK_SET_WAITERS(rwl, need_wait, set_wait)			\
do {									\
	unsigned long _tmp0_, _tmp1_;					\
									\
	__asm __volatile(						\
		"# BEGIN RWLOCK_SET_WAITERS\n"				\
		"1:	ldq_l	%0, %5		\n"			\
		"	and	%0, %3, %1	\n"			\
		"	beq	%1, 3f		\n"			\
		"	or	%0, %4, %0	\n"			\
		"	stq_c	%0, %2		\n"			\
		"	beq	%0, 2f		\n"			\
		"	mb			\n"			\
		"	br	3f		\n"			\
		"2:	br	1b		\n"			\
		"3:				\n"			\
		"	# END RWLOCK_SET_WAITERS"			\
		: "=&r" (_tmp0_), "=&r" (_tmp1_), "=m" ((rwl)->rwl_owner) \
		: "r" (need_wait), "r" (set_wait), "m" ((rwl)->rwl_owner) \
		: "memory");						\
} while (/*CONSTCOND*/0)

#endif /* _ALPHA_RWLOCK_IMPL_H_ */
