/*	$NetBSD: nospec.h,v 1.3 2021/12/19 01:45:38 riastradh Exp $	*/

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

#ifndef	_LINUX_NOSPEC_H_
#define	_LINUX_NOSPEC_H_

#include <sys/param.h>
#include <sys/types.h>

/*
 * array_index_nospec(i, n)
 *
 *	If i < n, return i; otherwise return 0.  Guarantees that
 *	speculative execution will behave the same even if the compiler
 *	can prove in the caller that i < n.
 */
static inline size_t
array_index_nospec(size_t i, size_t n)
{
	size_t j, diff, mask;

	/*
	 * Let j = i, but force the compiler to assume nothing about
	 * the value of j by issuing an asm block that is tagged as
	 * reading from and writing to j but actually does nothing.
	 */
	j = i;
	asm volatile ("" : "=r"(j) : "0"(j));

	/* If i >= n, then diff has the high bit set; otherwise clear.  */
	diff = n - 1 - j;

	/*
	 * If i >= n so that diff's high bit is set, mask = 0;
	 * otherwise mask = -1 = ~0.
	 */
	mask = (diff >> (NBBY*sizeof(diff) - 1)) - 1;

	return i & mask;
}

#endif	/* _LINUX_NOSPEC_H_ */
