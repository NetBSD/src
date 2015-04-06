/*	$NetBSD: cpuset.h,v 1.2.38.1 2015/04/06 15:17:59 skrll Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _MIPS_CPUSET_H_
#define	_MIPS_CPUSET_H_

#include <sys/atomic.h>

#define	CPUSET_SINGLE(cpu)		((__cpuset_t)1 << (cpu))

#if defined(__mips_o32)
#define	CPUSET_ADD(set, cpu)		atomic_or_32(&(set), CPUSET_SINGLE(cpu))
#define	CPUSET_DEL(set, cpu)		atomic_and_32(&(set), ~CPUSET_SINGLE(cpu))
#define	CPUSET_SUB(set1, set2)		atomic_and_32(&(set1), ~(set2))
#else
#define	CPUSET_ADD(set, cpu)		atomic_or_64(&(set), CPUSET_SINGLE(cpu))
#define	CPUSET_DEL(set, cpu)		atomic_and_64(&(set), ~CPUSET_SINGLE(cpu))
#define	CPUSET_SUB(set1, set2)		atomic_and_64(&(set1), ~(set2))
#endif
#define	CPUSET_EXCEPT(set, cpu)		((set) & ~CPUSET_SINGLE(cpu))

#define	CPUSET_HAS_P(set, cpu)		((set) & CPUSET_SINGLE(cpu))
#define	CPUSET_NEXT(set)		(ffs(set) - 1)

#define	CPUSET_EMPTY_P(set)		((set) == (__cpuset_t)0)
#define	CPUSET_EQUAL_P(set1, set2)	((set1) == (set2))
#define	CPUSET_CLEAR(set)		((set) = (__cpuset_t)0)
#define	CPUSET_ASSIGN(set1, set2)	((set1) = (set2))

#endif /* _MIPS_CPUSET_H_ */
