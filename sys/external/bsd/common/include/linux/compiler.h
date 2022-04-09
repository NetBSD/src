/*	$NetBSD: compiler.h,v 1.8 2022/04/09 23:43:31 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_COMPILER_H_
#define	_LINUX_COMPILER_H_

#include <sys/atomic.h>
#include <sys/cdefs.h>

#include <linux/stddef.h>

#include <asm/barrier.h>

#define	__printf	__printflike
#define	__user
#if __GNUC_PREREQ__(4,0)	/* not sure when but this will work */
#define	__must_check	__attribute__((warn_unused_result))
#else
#define	__must_check	/* nothing */
#endif
#define	__always_unused	__unused
#define	__maybe_unused	__unused
#define	noinline	__noinline
#define	__deprecated	/* nothing */
#define	__acquire(X)	/* nothing */
#define	__release(X)	/* nothing */

#define	barrier()	__insn_barrier()
#define	likely(X)	__predict_true(X)
#define	unlikely(X)	__predict_false(X)
#define	__same_type(X,Y)						      \
	__builtin_types_compatible_p(__typeof__(X), __typeof__(Y))
#define	__must_be_array(X)						      \
	BUILD_BUG_ON_ZERO(__same_type((X), &(X)[0]))

#define	READ_ONCE(X)	({						      \
	typeof(X) __read_once_tmp = (X);				      \
	membar_datadep_consumer();					      \
	__read_once_tmp;						      \
})

#define	WRITE_ONCE(X, V)	({					      \
	typeof(X) __write_once_tmp = (V);				      \
	(X) = __write_once_tmp;						      \
	__write_once_tmp;						      \
})

#define	smp_store_mb(X, V)	do {					      \
	WRITE_ONCE(X, V);						      \
	smp_mb();							      \
} while (0)

#define	smp_store_release(X, V)	do {					      \
	typeof(X) __smp_store_release_tmp = (V);			      \
	membar_release();						      \
	(X) = __write_once_tmp;						      \
} while (0)

#endif	/* _LINUX_COMPILER_H_ */
