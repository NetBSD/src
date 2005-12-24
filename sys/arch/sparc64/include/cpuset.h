/*	$NetBSD: cpuset.h,v 1.4 2005/12/24 20:07:37 perry Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _SPARC64_CPUSET_H_
#define	_SPARC64_CPUSET_H_

typedef	uint64_t cpuset_t;
extern volatile cpuset_t cpus_active;

#define	CPUSET_SINGLE(cpu)		((cpuset_t)1 << (cpu))

#define	CPUSET_ADD(set, cpu)		((set) |= CPUSET_SINGLE(cpu))
#define	CPUSET_DEL(set, cpu)		((set) &= ~CPUSET_SINGLE(cpu))
#define	CPUSET_SUB(set1, set2)		((set1) &= ~(set2))

#define	CPUSET_ALL(set)			((set) = (cpuset_t)-1)
#define	CPUSET_ALL_BUT(set, cpu)	((set) = ~CPUSET_SINGLE(cpu))

#define	CPUSET_HAS(set, cpu)		((set) & CPUSET_SINGLE(cpu))
#define	CPUSET_NEXT(set)		(ffs(set) - 1)

#define	CPUSET_EMPTY(set)		((set) == (cpuset_t)0)
#define	CPUSET_EQUAL(set1, set2)	((set1) == (set2))
#define	CPUSET_CLEAR(set)		((set) = (cpuset_t)0)
#define	CPUSET_ASSIGN(set1, set2)	((set1) = (set2))

#endif /* _SPARC64_CPUSET_H_ */
