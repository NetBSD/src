/*	$NetBSD: immintrin_ext.h,v 1.2 2024/07/15 13:59:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef	_SYS_CRYPTO_ARCH_X86_IMMINTRIN_EXT_H
#define	_SYS_CRYPTO_ARCH_X86_IMMINTRIN_EXT_H

#include <crypto/arch/x86/immintrin.h>

_INTRINSATTR
static __inline __m128i
_mm_loadu_epi8(const void *__p)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (__m128i)(*(const __v16qi_u *)__p);
#else
	return ((const struct { __m128i_u __v; } _PACKALIAS *)__p)->__v;
#endif
}

_INTRINSATTR
static __inline void
_mm_storeu_epi8(void *__p, __m128i __v)
{
#if defined(__GNUC__) && !defined(__clang__)
	*(__v16qi_u *)__p = (__v16qi_u)__v;
#else
	((struct { __m128i_u __v; } _PACKALIAS *)__p)->__v = __v;
#endif
}

#endif	/* _SYS_CRYPTO_ARCH_X86_IMMINTRIN_EXT_H */
