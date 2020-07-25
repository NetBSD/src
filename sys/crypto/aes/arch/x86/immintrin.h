/*	$NetBSD: immintrin.h,v 1.5 2020/07/25 22:45:10 riastradh Exp $	*/

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

#ifndef	_SYS_CRYPTO_AES_ARCH_X86_IMMINTRIN_H
#define	_SYS_CRYPTO_AES_ARCH_X86_IMMINTRIN_H

#include <sys/types.h>

/*
 * This kludgerous header file provides definitions for the Intel
 * intrinsics that work with GCC and Clang, because <immintrin.h> is
 * not available during the kernel build and arranging to make it
 * available is complicated.  Please fix this properly!
 */

#if defined(__GNUC__) && !defined(__clang__)

#define	_INTRINSATTR							      \
	__attribute__((__gnu_inline__, __always_inline__, __artificial__))
#define	_PACKALIAS

typedef float __m128 __attribute__((__vector_size__(16), __may_alias__));
typedef long long __m128i __attribute__((__vector_size__(16), __may_alias__));
typedef long long __m128i_u
    __attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));
typedef long long __v2di __attribute__((__vector_size__(16)));
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));
typedef int __v4si __attribute__((__vector_size__(16)));
typedef unsigned __v4su __attribute__((__vector_size__(16)));
typedef float __v4sf __attribute__((__vector_size__(16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));

#elif defined(__clang__)

typedef float __m128 __attribute__((__vector_size__(16), __aligned__(16)));
typedef long long __m128i
    __attribute__((__vector_size__(16), __aligned__(16)));
typedef long long __m128i_u
    __attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));
typedef long long __v2di __attribute__((__vector_size__(16)));
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));
typedef int __v4si __attribute__((__vector_size__(16)));
typedef unsigned __v4su __attribute__((__vector_size__(16)));
typedef float __v4sf __attribute__((__vector_size__(16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));

#define	_INTRINSATTR							      \
	__attribute__((__always_inline__, __nodebug__, __target__("sse2"),    \
		__min_vector_width__(128)))
#define	_PACKALIAS							      \
	__attribute__((__packed__, __may_alias__))

#else

#error Please teach me how to do Intel intrinsics for your compiler!

#endif

#define	_SSSE3_ATTR	__attribute__((target("ssse3")))

_INTRINSATTR
static __inline __m128i
_mm_add_epi32(__m128i __a, __m128i __b)
{
	return (__m128i)((__v4su)__a + (__v4su)__b);
}

#if defined(__GNUC__) && !defined(__clang__)
#define	_mm_alignr_epi8(hi,lo,bytes)					      \
	(__m128i)__builtin_ia32_palignr128((__v2di)(__m128i)(hi),	      \
	    (__v2di)(__m128i)(lo), 8*(int)(bytes))
#elif defined(__clang__)
#define	_mm_alignr_epi8(hi,lo,bytes)					      \
	(__m128i)__builtin_ia32_palignr128((__v16qi)(__m128i)(hi),	      \
	    (__v16qi)(__m128i)(lo), (int)(bytes))
#endif

_INTRINSATTR
static __inline __m128
_mm_load1_ps(const float *__p)
{
	return __extension__ (__m128)(__v4sf) { *__p, *__p, *__p, *__p };
}

_INTRINSATTR
static __inline __m128i
_mm_loadu_si128(const __m128i_u *__p)
{
	return ((const struct { __m128i_u __v; } _PACKALIAS *)__p)->__v;
}

_INTRINSATTR
static __inline __m128i
_mm_loadu_si32(const void *__p)
{
	int32_t __v = ((const struct { int32_t __v; } _PACKALIAS *)__p)->__v;
	return __extension__ (__m128i)(__v4si){ __v, 0, 0, 0 };
}

_INTRINSATTR
static __inline __m128i
_mm_loadu_si64(const void *__p)
{
	int64_t __v = ((const struct { int64_t __v; } _PACKALIAS *)__p)->__v;
	return __extension__ (__m128i)(__v2di){ __v, 0 };
}

_INTRINSATTR
static __inline __m128i
_mm_load_si128(const __m128i *__p)
{
	return *__p;
}

_INTRINSATTR
static __inline __m128
_mm_movehl_ps(__m128 __v0, __m128 __v1)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (__m128)__builtin_ia32_movhlps((__v4sf)__v0, (__v4sf)__v1);
#elif defined(__clang__)
	return __builtin_shufflevector((__v4sf)__v0, (__v4sf)__v1, 6,7,2,3);
#endif
}

_INTRINSATTR
static __inline __m128
_mm_movelh_ps(__m128 __v0, __m128 __v1)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (__m128)__builtin_ia32_movlhps((__v4sf)__v0, (__v4sf)__v1);
#elif defined(__clang__)
	return __builtin_shufflevector((__v4sf)__v0, (__v4sf)__v1, 0,1,4,5);
#endif
}

_INTRINSATTR
static __inline __m128i
_mm_set1_epi16(int16_t __v)
{
	return __extension__ (__m128i)(__v8hi){
	    __v, __v, __v, __v, __v, __v, __v, __v
	};
}

_INTRINSATTR
static __inline __m128i
_mm_set1_epi32(int32_t __v)
{
	return __extension__ (__m128i)(__v4si){ __v, __v, __v, __v };
}

_INTRINSATTR
static __inline __m128i
_mm_set1_epi64x(int64_t __v)
{
	return __extension__ (__m128i)(__v2di){ __v, __v };
}

_INTRINSATTR
static __inline __m128i
_mm_set_epi32(int32_t __v3, int32_t __v2, int32_t __v1, int32_t __v0)
{
	return __extension__ (__m128i)(__v4si){ __v0, __v1, __v2, __v3 };
}

_INTRINSATTR
static __inline __m128i
_mm_set_epi64x(int64_t __v1, int64_t __v0)
{
	return __extension__ (__m128i)(__v2di){ __v0, __v1 };
}

_INTRINSATTR
static __inline __m128
_mm_setzero_ps(void)
{
	return __extension__ (__m128){ 0, 0, 0, 0 };
}

_INTRINSATTR
static __inline __m128i
_mm_setzero_si128(void)
{
	return _mm_set1_epi64x(0);
}

_INTRINSATTR _SSSE3_ATTR
static __inline __m128i
_mm_shuffle_epi8(__m128i __vtbl, __m128i __vidx)
{
	return (__m128i)__builtin_ia32_pshufb128((__v16qi)__vtbl,
	    (__v16qi)__vidx);
}

#define	_mm_shuffle_epi32(v,m)						      \
	(__m128i)__builtin_ia32_pshufd((__v4si)(__m128i)(v), (int)(m))

#define	_mm_shuffle_ps(x,y,m)						      \
	(__m128)__builtin_ia32_shufps((__v4sf)(__m128)(x),		      \
	    (__v4sf)(__m128)(y), (int)(m))				      \

_INTRINSATTR
static __inline __m128i
_mm_slli_epi32(__m128i __v, uint8_t __bits)
{
	return (__m128i)__builtin_ia32_pslldi128((__v4si)__v, (int)__bits);
}

_INTRINSATTR
static __inline __m128i
_mm_slli_epi64(__m128i __v, uint8_t __bits)
{
	return (__m128i)__builtin_ia32_psllqi128((__v2di)__v, (int)__bits);
}

#if defined(__GNUC__) && !defined(__clang__)
#define	_mm_slli_si128(v,bytes)						      \
	(__m128i)__builtin_ia32_pslldqi128((__v2di)(__m128i)(v),	      \
	    8*(int)(bytes))
#elif defined(__clang__)
#define	_mm_slli_si128(v,bytes)						      \
	(__m128i)__builtin_ia32_pslldqi128_byteshift((__v2di)(__m128i)(v),    \
	    (int)(bytes))
#endif

_INTRINSATTR
static __inline __m128i
_mm_srli_epi32(__m128i __v, uint8_t __bits)
{
	return (__m128i)__builtin_ia32_psrldi128((__v4si)__v, (int)__bits);
}

_INTRINSATTR
static __inline __m128i
_mm_srli_epi64(__m128i __v, uint8_t __bits)
{
	return (__m128i)__builtin_ia32_psrlqi128((__v2di)__v, (int)__bits);
}

#if defined(__GNUC__) && !defined(__clang__)
#define	_mm_srli_si128(v,bytes)						      \
	(__m128i)__builtin_ia32_psrldqi128((__m128i)(v), 8*(int)(bytes))
#elif defined(__clang__)
#define	_mm_srli_si128(v,bytes)						      \
	(__m128i)__builtin_ia32_psrldqi128_byteshift((__v2di)(__m128i)(v),    \
	    (int)(bytes));
#endif

_INTRINSATTR
static __inline void
_mm_storeu_si128(__m128i_u *__p, __m128i __v)
{
	((struct { __m128i_u __v; } _PACKALIAS *)__p)->__v = __v;
}

_INTRINSATTR
static __inline void
_mm_storeu_si32(void *__p, __m128i __v)
{
	((struct { int32_t __v; } _PACKALIAS *)__p)->__v = ((__v4si)__v)[0];
}

_INTRINSATTR
static __inline void
_mm_storeu_si64(void *__p, __m128i __v)
{
	((struct { int64_t __v; } _PACKALIAS *)__p)->__v = ((__v2di)__v)[0];
}

_INTRINSATTR
static __inline void
_mm_store_si128(__m128i *__p, __m128i __v)
{
	*__p = __v;
}

_INTRINSATTR
static __inline __m128i
_mm_sub_epi64(__m128i __x, __m128i __y)
{
	return (__m128i)((__v2du)__x - (__v2du)__y);
}

_INTRINSATTR
static __inline __m128i
_mm_unpackhi_epi32(__m128i __lo, __m128i __hi)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (__m128i)__builtin_ia32_punpckhdq128((__v4si)__lo,
	    (__v4si)__hi);
#elif defined(__clang__)
	return (__m128i)__builtin_shufflevector((__v4si)__lo, (__v4si)__hi,
	    2,6,3,7);
#endif
}

_INTRINSATTR
static __inline __m128i
_mm_unpacklo_epi32(__m128i __lo, __m128i __hi)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (__m128i)__builtin_ia32_punpckldq128((__v4si)__lo,
	    (__v4si)__hi);
#elif defined(__clang__)
	return (__m128i)__builtin_shufflevector((__v4si)__lo, (__v4si)__hi,
	    0,4,1,5);
#endif
}

_INTRINSATTR
static __inline __m128i
_mm_unpacklo_epi64(__m128i __lo, __m128i __hi)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (__m128i)__builtin_ia32_punpcklqdq128((__v2di)__lo,
	    (__v2di)__hi);
#elif defined(__clang__)
	return (__m128i)__builtin_shufflevector((__v2di)__lo, (__v2di)__hi,
	    0,2);
#endif
}

#endif	/* _SYS_CRYPTO_AES_ARCH_X86_IMMINTRIN_H */
