/*	$NetBSD: overflow.h,v 1.2 2021/12/19 12:20:53 riastradh Exp $	*/

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

#ifndef _LINUX_OVERFLOW_H_
#define _LINUX_OVERFLOW_H_

#include <sys/types.h>

#include <lib/libkern/libkern.h>	/* offsetof */

#define	check_mul_overflow(a, b, res)	__builtin_mul_overflow(a, b, res)
#define	check_add_overflow(a, b, res)	__builtin_add_overflow(a, b, res)

/* return x*y saturated at SIZE_MAX */
static inline size_t
array_size(size_t x, size_t y)
{
	size_t xy;

	if (check_mul_overflow(x, y, &xy))
		return SIZE_MAX;
	return xy;
}

/* return x*y*z saturated at SIZE_MAX */
static inline size_t
array3_size(size_t x, size_t y, size_t z)
{
	size_t xy, xyz;

	if (check_mul_overflow(x, y, &xy))
		return SIZE_MAX;
	if (check_mul_overflow(xy, z, &xyz))
		return SIZE_MAX;
	return xyz;
}

/* return basesize + elemsize*nelem saturated at SIZE_MAX */
static inline size_t
__struct_size(size_t basesize, size_t elemsize, size_t nelem)
{
	size_t arraysize, totalsize;

	KASSERT(elemsize);
	if ((arraysize = array_size(elemsize, nelem)) == SIZE_MAX)
		return SIZE_MAX;
	if (check_add_overflow(basesize, arraysize, &totalsize))
		return SIZE_MAX;
	return totalsize;
}

#define	struct_size(p, member, n)					      \
({									      \
	CTASSERT(sizeof(*(p)) >= offsetof(__typeof__(*(p)), member));	      \
	__struct_size(sizeof(*(p)), sizeof((p)->member[0]), (n));	      \
})

#endif  /* _LINUX_OVERFLOW_H_ */
