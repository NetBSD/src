/*	$NetBSD: kernel.h,v 1.1.2.15 2013/07/24 02:56:17 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _LINUX_KERNEL_H_
#define _LINUX_KERNEL_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#define	__printf	__printflike
#define	__user
#define	__must_check	/* __attribute__((warn_unused_result)), if GCC */

#define	barrier()	__insn_barrier()
#define	unlikely(X)	__predict_false(X)

#define	uninitialized_var(x)	x

#define	container_of(PTR, TYPE, FIELD)					\
	((void)sizeof((PTR) -						\
		&((TYPE *)(((char *)(PTR)) -				\
		    offsetof(TYPE, FIELD)))->FIELD),			\
	    ((TYPE *)(((char *)(PTR)) - offsetof(TYPE, FIELD))))

#define	ARRAY_SIZE(ARRAY)	__arraycount(ARRAY)

#define	swap(X, Y)	do						\
{									\
	/* XXX Kludge for type-safety.  */				\
	if (&(X) != &(Y)) {						\
		CTASSERT(sizeof(X) == sizeof(Y));			\
		/* XXX Can't do this much better without typeof.  */	\
		char __swap_tmp[sizeof(X)];				\
		(void)memcpy(__swap_tmp, &(X), sizeof(X));		\
		(void)memcpy(&(X), &(Y), sizeof(X));			\
		(void)memcpy(&(Y), __swap_tmp, sizeof(X));		\
	}								\
} while (0)

static inline int64_t
abs64(int64_t x)
{
	return (x < 0? (-x) : x);
}

#endif  /* _LINUX_KERNEL_H_ */
