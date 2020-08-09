/*	$NetBSD: arm_neon.h,v 1.6 2020/08/09 02:49:38 riastradh Exp $	*/

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

#ifndef	_SYS_CRYPTO_CHACHA_ARCH_ARM_ARM_NEON_H
#define	_SYS_CRYPTO_CHACHA_ARCH_ARM_ARM_NEON_H

#if defined(__GNUC__) && !defined(__clang__)

#define	_INTRINSATTR							      \
	__extension__							      \
	__attribute__((__always_inline__, __gnu_inline__, __artificial__))

#ifdef __aarch64__
typedef __Int32x4_t int32x4_t;
typedef __Int64x2_t int64x2_t;
typedef __Int8x16_t int8x16_t;
typedef __Uint16x8_t uint16x8_t;
typedef __Uint32x4_t uint32x4_t;
typedef __Uint64x2_t uint64x2_t;
typedef __Uint8x16_t uint8x16_t;
typedef struct { uint8x16_t val[2]; } uint8x16x2_t;
#else
typedef __simd128_int32_t int32x4_t;
typedef __simd128_int64_t int64x2_t;
typedef __simd128_int8_t int8x16_t;
typedef __simd128_uint16_t uint16x8_t;
typedef __simd128_uint32_t uint32x4_t;
typedef __simd128_uint64_t uint64x2_t;
typedef __simd128_uint8_t uint8x16_t;

typedef __simd64_int8_t int8x8_t;
typedef __simd64_uint8_t uint8x8_t;
typedef __builtin_neon_udi uint64x1_t;
typedef struct { uint8x8_t val[2]; } uint8x8x2_t;
typedef struct { uint8x16_t val[2]; } uint8x16x2_t;
#endif

#if defined(__AARCH64EB__)
#define	__neon_lane_index(__v, __i)	(__arraycount(__v) - 1 - (__i))
#define	__neon_laneq_index(__v, __i)	(__arraycount(__v) - 1 - (__i))
#elif defined(__ARM_BIG_ENDIAN)
#define	__neon_lane_index(__v, __i)	((__i) ^ (__arraycount(__v) - 1))
#define	__neon_laneq_index(__v, __i)	((__i) ^ (__arraycount(__v)/2 - 1))
#else
#define	__neon_lane_index(__v, __i)	(__i)
#define	__neon_laneq_index(__v, __i)	(__i)
#endif

#elif defined(__clang__)

#define	_INTRINSATTR							      \
	__attribute__((__always_inline__, __nodebug__))

typedef __attribute__((neon_vector_type(16))) int8_t int8x16_t;
typedef __attribute__((neon_vector_type(2))) int64_t int64x2_t;
typedef __attribute__((neon_vector_type(4))) int32_t int32x4_t;

typedef __attribute__((neon_vector_type(16))) uint8_t uint8x16_t;
typedef __attribute__((neon_vector_type(2))) uint64_t uint64x2_t;
typedef __attribute__((neon_vector_type(4))) uint32_t uint32x4_t;
typedef __attribute__((neon_vector_type(8))) uint16_t uint16x8_t;

typedef __attribute__((neon_vector_type(8))) int8_t int8x8_t;

typedef __attribute__((neon_vector_type(8))) uint8_t uint8x8_t;

typedef struct { uint8x8_t val[2]; } uint8x8x2_t;
typedef struct { uint8x16_t val[2]; } uint8x16x2_t;

#ifdef __LITTLE_ENDIAN__
#define	__neon_lane_index(__v, __i)	__i
#define	__neon_laneq_index(__v, __i)	__i
#else
#define	__neon_lane_index(__v, __i)	(__arraycount(__v) - 1 - __i)
#define	__neon_laneq_index(__v, __i)	(__arraycount(__v) - 1 - __i)
#endif

#else

#error Teach me how to neon in your compile!

#endif

_INTRINSATTR
static __inline uint32x4_t
vaddq_u32(uint32x4_t __v0, uint32x4_t __v1)
{
	return __v0 + __v1;
}

_INTRINSATTR
static __inline uint32x4_t
vcltq_s32(int32x4_t __v0, int32x4_t __v1)
{
	return (uint32x4_t)(__v0 < __v1);
}

_INTRINSATTR
static __inline int32x4_t
vdupq_n_s32(int32_t __x)
{
	return (int32x4_t) { __x, __x, __x, __x };
}

_INTRINSATTR
static __inline uint32x4_t
vdupq_n_u32(uint32_t __x)
{
	return (uint32x4_t) { __x, __x, __x, __x };
}

_INTRINSATTR
static __inline uint8x16_t
vdupq_n_u8(uint8_t __x)
{
	return (uint8x16_t) {
		__x, __x, __x, __x, __x, __x, __x, __x,
		__x, __x, __x, __x, __x, __x, __x, __x,
	};
}

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint32x4_t
vextq_u32(uint32x4_t __lo, uint32x4_t __hi, uint8_t __i)
{
#if defined(__AARCH64EB__) || defined(__ARM_BIG_ENDIAN)
	return __builtin_shuffle(__hi, __lo,
	    (uint32x4_t) { 4 - __i, 5 - __i, 6 - __i, 7 - __i });
#else
	return __builtin_shuffle(__lo, __hi,
	    (uint32x4_t) { __i + 0, __i + 1, __i + 2, __i + 3 });
#endif
}
#elif defined(__clang__)
#ifdef __LITTLE_ENDIAN__
#define	vextq_u32(__lo, __hi, __i)					      \
	(uint32x4_t)__builtin_neon_vextq_v((int8x16_t)(__lo),		      \
	    (int8x16_t)(__hi), (__i), 50)
#else
#define	vextq_u32(__lo, __hi, __i) (					      \
{									      \
	uint32x4_t __tlo = (__lo);					      \
	uint32x4_t __thi = (__hi);					      \
	uint32x4_t __lo_r = __builtin_shufflevector(__tlo, __tlo, 3,2,1,0);   \
	uint32x4_t __hi_r = __builtin_shufflevector(__thi, __thi, 3,2,1,0);   \
	uint32x4_t __r = __builtin_neon_vextq_v((int8x16_t)__lo_r,	      \
	    (int8x16_t)__hi_r, __i, 50);				      \
	__builtin_shufflevector(__r, __r, 3,2,1,0);			      \
})
#endif	/* __LITTLE_ENDIAN__ */
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint8x16_t
vextq_u8(uint8x16_t __lo, uint8x16_t __hi, uint8_t __i)
{
#ifdef __aarch64__
#if defined(__AARCH64EB__)
	return __builtin_shuffle(__hi, __lo,
	    (uint8x16_t) {
		16 - __i, 17 - __i, 18 - __i, 19 - __i,
		20 - __i, 21 - __i, 22 - __i, 23 - __i,
		24 - __i, 25 - __i, 26 - __i, 27 - __i,
		28 - __i, 29 - __i, 30 - __i, 31 - __i,
	});
#else
	return __builtin_shuffle(__lo, __hi,
	    (uint8x16_t) {
		__i +  0, __i +  1, __i +  2, __i +  3,
		__i +  4, __i +  5, __i +  6, __i +  7,
		__i +  8, __i +  9, __i + 10, __i + 11,
		__i + 12, __i + 13, __i + 14, __i + 15,
	});
#endif
#else
	return (uint8x16_t)__builtin_neon_vextv16qi((int8x16_t)__lo,
	    (int8x16_t)__hi, __i);
#endif
}
#elif defined(__clang__)
#ifdef __LITTLE_ENDIAN__
#define	vextq_u8(__lo, __hi, __i)					      \
	(uint8x16_t)__builtin_neon_vextq_v((int8x16_t)(__lo),		      \
	    (int8x16_t)(__hi), (__i), 48)
#else
#define	vextq_u8(__lo, __hi, __i) (					      \
{									      \
	uint8x16_t __tlo = (__lo);					      \
	uint8x16_t __thi = (__hi);					      \
	uint8x16_t __lo_r = __builtin_shufflevector(__tlo, __tlo,	      \
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);			      \
	uint8x16_t __hi_r = __builtin_shufflevector(__thi, __thi,	      \
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);			      \
	uint8x16_t __r = __builtin_neon_vextq_v((int8x16_t)__lo_r,	      \
	    (int8x16_t)__hi_r, (__i), 48);				      \
	__builtin_shufflevector(__r, __r,				      \
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);			      \
})
#endif	/* __LITTLE_ENDIAN */
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint32_t
vgetq_lane_u32(uint32x4_t __v, uint8_t __i)
{
#ifdef __aarch64__
	return __v[__i];
#else
	return (uint32_t)__builtin_neon_vget_laneuv4si((int32x4_t)__v, __i);
#endif
}
#elif defined(__clang__)
#define	vgetq_lane_u32(__v, __i)					      \
	(uint32_t)__builtin_neon_vgetq_lane_i32((int32x4_t)(__v),	      \
	    __neon_laneq_index(__v, __i))
#endif

_INTRINSATTR
static __inline uint32x4_t
vld1q_u32(const uint32_t *__p32)
{
#if defined(__GNUC__) && !defined(__clang__)
#ifdef __aarch64__
	const __builtin_aarch64_simd_si *__p =
	    (const __builtin_aarch64_simd_si *)__p32;

	return (uint32x4_t)__builtin_aarch64_ld1v4si(__p);
#else
	const __builtin_neon_si *__p = (const __builtin_neon_si *)__p32;

	return (uint32x4_t)__builtin_neon_vld1v4si(__p);
#endif
#elif defined(__clang__)
	uint32x4_t __v = (uint32x4_t)__builtin_neon_vld1q_v(__p32, 50);
#ifndef __LITTLE_ENDIAN__
	__v = __builtin_shufflevector(__v, __v, 3,2,1,0);
#endif
	return __v;
#endif
}

_INTRINSATTR
static __inline uint8x16_t
vld1q_u8(const uint8_t *__p8)
{
#if defined(__GNUC__) && !defined(__clang__)
#ifdef __aarch64__
	const __builtin_aarch64_simd_qi *__p =
	    (const __builtin_aarch64_simd_qi *)__p8;

	return (uint8x16_t)__builtin_aarch64_ld1v16qi(__p);
#else
	const __builtin_neon_qi *__p = (const __builtin_neon_qi *)__p8;

	return (uint8x16_t)__builtin_neon_vld1v16qi(__p);
#endif
#elif defined(__clang__)
	uint8x16_t __v = (uint8x16_t)__builtin_neon_vld1q_v(__p8, 48);
#ifndef __LITTLE_ENDIAN__
	__v = __builtin_shufflevector(__v, __v,
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
#endif
	return __v;
#endif
}

_INTRINSATTR
static __inline uint8x16_t
vqtbl1q_u8(uint8x16_t __tab, uint8x16_t __idx)
{
#if defined(__GNUC__) && !defined(__clang__)
#ifdef __aarch64__
	uint8x16_t __res;
	__asm__("tbl %0.16b, {%1.16b}, %2.16b"
	    : "=w"(__res) : "w"(__tab), "w"(__idx));
	return __res;
#else
	/*
	 * No native ARMv7 NEON instruction for this, so do it via two
	 * half-width TBLs instead (vtbl2_u8 equivalent).
	 */
	uint64x2_t __tab64 = (uint64x2_t)__tab;
	uint8x8_t __tablo = (uint8x8_t)__tab64[0];
	uint8x8_t __tabhi = (uint8x8_t)__tab64[1];
	uint8x8x2_t __tab8x8x2 = { { __tablo, __tabhi } };
	union {
		uint8x8x2_t __u8x8x2;
		__builtin_neon_ti __ti;
	} __u = { __tab8x8x2 };
	uint64x2_t __idx64, __out64;
	int8x8_t __idxlo, __idxhi, __outlo, __outhi;

	__idx64 = (uint64x2_t)__idx;
	__idxlo = (int8x8_t)__idx64[0];
	__idxhi = (int8x8_t)__idx64[1];
	__outlo = (int8x8_t)__builtin_neon_vtbl2v8qi(__u.__ti, __idxlo);
	__outhi = (int8x8_t)__builtin_neon_vtbl2v8qi(__u.__ti, __idxhi);
	__out64 = (uint64x2_t) { (uint64x1_t)__outlo, (uint64x1_t)__outhi };

	return (uint8x16_t)__out64;
#endif
#elif defined(__clang__)
#ifndef __LITTLE_ENDIAN__
	__tab = __builtin_shufflevector(__tab, __tab,
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
	__idx = __builtin_shufflevector(__idx, __idx,
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
#endif
	uint8x16_t __r;
#ifdef __aarch64__
	__r = __builtin_neon_vqtbl1q_v((int8x16_t)__tab, (int8x16_t)__idx, 48);
#else
	uint64x2_t __tab64 = (uint64x2_t)__tab;
	uint8x8_t __tablo = (uint8x8_t)__tab64[0];
	uint8x8_t __tabhi = (uint8x8_t)__tab64[1];
	uint64x2_t __idx64, __out64;
	int8x8_t __idxlo, __idxhi, __outlo, __outhi;

	__idx64 = (uint64x2_t)__idx;
	__idxlo = (int8x8_t)__idx64[0];
	__idxhi = (int8x8_t)__idx64[1];
	__outlo = (uint8x8_t)__builtin_neon_vtbl2_v((int8x8_t)__tablo,
	    (int8x8_t)__tabhi, (int8x8_t)__idxlo, 16);
	__outhi = (uint8x8_t)__builtin_neon_vtbl2_v((int8x8_t)__tablo,
	    (int8x8_t)__tabhi, (int8x8_t)__idxhi, 16);
	__out64 = (uint64x2_t) { (uint64_t)__outlo, (uint64_t)__outhi };
	__r = (uint8x16_t)__out64;
#endif
#ifndef __LITTLE_ENDIAN__
	__r = __builtin_shufflevector(__r, __r,
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
#endif
	return __r;
#endif
}

_INTRINSATTR
static __inline int32x4_t
vreinterpretq_s32_u8(uint8x16_t __v)
{
	return (int32x4_t)__v;
}

_INTRINSATTR
static __inline uint16x8_t
vreinterpretq_u16_u32(uint32x4_t __v)
{
	return (uint16x8_t)__v;
}

_INTRINSATTR
static __inline uint32x4_t
vreinterpretq_u32_u16(uint16x8_t __v)
{
	return (uint32x4_t)__v;
}

_INTRINSATTR
static __inline uint32x4_t
vreinterpretq_u32_u64(uint64x2_t __v)
{
	return (uint32x4_t)__v;
}

_INTRINSATTR
static __inline uint32x4_t
vreinterpretq_u32_u8(uint8x16_t __v)
{
	return (uint32x4_t)__v;
}

_INTRINSATTR
static __inline uint64x2_t
vreinterpretq_u64_u32(uint32x4_t __v)
{
	return (uint64x2_t)__v;
}

_INTRINSATTR
static __inline uint64x2_t
vreinterpretq_u64_u8(uint8x16_t __v)
{
	return (uint64x2_t)__v;
}

_INTRINSATTR
static __inline uint8x16_t
vreinterpretq_u8_s32(int32x4_t __v)
{
	return (uint8x16_t)__v;
}

_INTRINSATTR
static __inline uint8x16_t
vreinterpretq_u8_u32(uint32x4_t __v)
{
	return (uint8x16_t)__v;
}

_INTRINSATTR
static __inline uint8x16_t
vreinterpretq_u8_u64(uint64x2_t __v)
{
	return (uint8x16_t)__v;
}

_INTRINSATTR
static __inline uint16x8_t
vrev32q_u16(uint16x8_t __v)
{
#if defined(__GNUC__) && !defined(__clang__)
	return __builtin_shuffle(__v, (uint16x8_t) { 1,0, 3,2, 5,4, 7,6 });
#elif defined(__clang__)
	return __builtin_shufflevector(__v, __v,  1,0, 3,2, 5,4, 7,6);
#endif
}

_INTRINSATTR
static __inline uint8x16_t
vrev32q_u8(uint8x16_t __v)
{
#if defined(__GNUC__) && !defined(__clang__)
	return __builtin_shuffle(__v,
	    (uint8x16_t) { 3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12 });
#elif defined(__clang__)
	return __builtin_shufflevector(__v, __v,
	    3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12);
#endif
}

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint32x4_t
vsetq_lane_u32(uint32_t __x, uint32x4_t __v, uint8_t __i)
{
	__v[__neon_laneq_index(__v, __i)] = __x;
	return __v;
}
#elif defined(__clang__)
#define	vsetq_lane_u32(__x, __v, __i)					      \
	(uint32x4_t)__builtin_neon_vsetq_lane_i32((__x), (int32x4_t)(__v),    \
	    __neon_laneq_index(__v, __i))
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint64x2_t
vsetq_lane_u64(uint64_t __x, uint64x2_t __v, uint8_t __i)
{
	__v[__neon_laneq_index(__v, __i)] = __x;
	return __v;
}
#elif defined(__clang__)
#define	vsetq_lane_u64(__x, __v, __i)					      \
	(uint64x2_t)__builtin_neon_vsetq_lane_i64((__x), (int64x2_t)(__v),    \
	    __neon_laneq_index(__v, __i));
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline int32x4_t
vshlq_n_s32(int32x4_t __v, uint8_t __bits)
{
#ifdef __aarch64__
	return (int32x4_t)__builtin_aarch64_ashlv4si(__v, __bits);
#else
	return (int32x4_t)__builtin_neon_vshl_nv4si(__v, __bits);
#endif
}
#elif defined(__clang__)
#define	vshlq_n_s32(__v, __bits)					      \
	(int32x4_t)__builtin_neon_vshlq_n_v((int32x4_t)(__v), (__bits), 34)
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint32x4_t
vshlq_n_u32(uint32x4_t __v, uint8_t __bits)
{
#ifdef __aarch64__
	return (uint32x4_t)__builtin_aarch64_ashlv4si((int32x4_t)__v, __bits);
#else
	return (uint32x4_t)__builtin_neon_vshl_nv4si((int32x4_t)__v, __bits);
#endif
}
#elif defined(__clang__)
#define	vshlq_n_u32(__v, __bits)					      \
	(uint32x4_t)__builtin_neon_vshlq_n_v((int32x4_t)(__v), (__bits), 50)
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint32x4_t
vshrq_n_u32(uint32x4_t __v, uint8_t __bits)
{
#ifdef __aarch64__
	return (uint32x4_t)__builtin_aarch64_lshrv4si((int32x4_t)__v, __bits);
#else
	return (uint32x4_t)__builtin_neon_vshru_nv4si((int32x4_t)__v, __bits);
#endif
}
#elif defined(__clang__)
#define	vshrq_n_u32(__v, __bits)					      \
	(uint32x4_t)__builtin_neon_vshrq_n_v((int32x4_t)(__v), (__bits), 50)
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint8x16_t
vshrq_n_u8(uint8x16_t __v, uint8_t __bits)
{
#ifdef __aarch64__
	return (uint8x16_t)__builtin_aarch64_lshrv16qi((int8x16_t)__v, __bits);
#else
	return (uint8x16_t)__builtin_neon_vshru_nv16qi((int8x16_t)__v, __bits);
#endif
}
#elif defined(__clang__)
#define	vshrq_n_u8(__v, __bits)						      \
	(uint8x16_t)__builtin_neon_vshrq_n_v((int8x16_t)(__v), (__bits), 48)
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline int32x4_t
vsliq_n_s32(int32x4_t __vins, int32x4_t __vsh, uint8_t __bits)
{
#ifdef __aarch64__
	return (int32x4_t)__builtin_aarch64_ssli_nv4si(__vins, __vsh, __bits);
#else
	return (int32x4_t)__builtin_neon_vsli_nv4si(__vins, __vsh, __bits);
#endif
}
#elif defined(__clang__)
#ifdef __LITTLE_ENDIAN__
#define	vsliq_n_s32(__vins, __vsh, __bits)				      \
	(int32x4_t)__builtin_neon_vsliq_n_v((int32x4_t)(__vins),	      \
	    (int32x4_t)(__vsh), (__bits), 34)
#else
#define	vsliq_n_s32(__vins, __vsh, __bits) (				      \
{									      \
	int32x4_t __tvins = (__vins);					      \
	int32x4_t __tvsh = (__vsh);					      \
	uint8_t __tbits = (__bits);					      \
	int32x4_t __vins_r = __builtin_shufflevector(__tvins, __tvins,	      \
	    3,2,1,0);							      \
	int32x4_t __vsh_r = __builtin_shufflevector(__tvsh, __tvsh,	      \
	    3,2,1,0);							      \
	int32x4_t __r = __builtin_neon_vsliq_n_v(__tvins, __tvsh, __tbits,    \
	    34);							      \
	__builtin_shufflevector(__r, __r, 3,2,1,0);			      \
})
#endif	/* __LITTLE_ENDIAN__ */
#endif

#if defined(__GNUC__) && !defined(__clang__)
_INTRINSATTR
static __inline uint32x4_t
vsriq_n_u32(uint32x4_t __vins, uint32x4_t __vsh, uint8_t __bits)
{
#ifdef __aarch64__
	return __builtin_aarch64_usri_nv4si_uuus(__vins, __vsh, __bits);
#else
	return (uint32x4_t)__builtin_neon_vsri_nv4si((int32x4_t)__vins,
	    (int32x4_t)__vsh, __bits);
#endif
}
#elif defined(__clang__)
#ifdef __LITTLE_ENDIAN__
#define	vsriq_n_u32(__vins, __vsh, __bits)				      \
	(int32x4_t)__builtin_neon_vsriq_n_v((int32x4_t)(__vins),	      \
	    (int32x4_t)(__vsh), (__bits), 34)
#else
#define	vsriq_n_s32(__vins, __vsh, __bits) (				      \
{									      \
	int32x4_t __tvins = (__vins);					      \
	int32x4_t __tvsh = (__vsh);					      \
	uint8_t __tbits = (__bits);					      \
	int32x4_t __vins_r = __builtin_shufflevector(__tvins, __tvins,	      \
	    3,2,1,0);							      \
	int32x4_t __vsh_r = __builtin_shufflevector(__tvsh, __tvsh,	      \
	    3,2,1,0);							      \
	int32x4_t __r = __builtin_neon_vsriq_n_v(__tvins, __tvsh, __tbits,    \
	    34);							      \
	__builtin_shufflevector(__r, __r, 3,2,1,0);			      \
})
#endif
#endif

_INTRINSATTR
static __inline void
vst1q_u32(uint32_t *__p32, uint32x4_t __v)
{
#if defined(__GNUC__) && !defined(__clang__)
#ifdef __aarch64__
	__builtin_aarch64_simd_si *__p = (__builtin_aarch64_simd_si *)__p32;

	__builtin_aarch64_st1v4si(__p, (int32x4_t)__v);
#else
	__builtin_neon_si *__p = (__builtin_neon_si *)__p32;

	__builtin_neon_vst1v4si(__p, (int32x4_t)__v);
#endif
#elif defined(__clang__)
#ifndef __LITTLE_ENDIAN__
	__v = __builtin_shufflevector(__v, __v, 3,2,1,0);
#endif
	__builtin_neon_vst1q_v(__p32, __v, 50);
#endif
}

_INTRINSATTR
static __inline void
vst1q_u8(uint8_t *__p8, uint8x16_t __v)
{
#if defined(__GNUC__) && !defined(__clang__)
#ifdef __aarch64__
	__builtin_aarch64_simd_qi *__p = (__builtin_aarch64_simd_qi *)__p8;

	__builtin_aarch64_st1v16qi(__p, (int8x16_t)__v);
#else
	__builtin_neon_qi *__p = (__builtin_neon_qi *)__p8;

	__builtin_neon_vst1v16qi(__p, (int8x16_t)__v);
#endif
#elif defined(__clang__)
#ifndef __LITTLE_ENDIAN__
	__v = __builtin_shufflevector(__v, __v,
	    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
#endif
	__builtin_neon_vst1q_v(__p8, __v, 48);
#endif
}

#ifndef __aarch64__		/* XXX */

_INTRINSATTR
static __inline uint8x8_t
vtbl1_u8(uint8x8_t __tab, uint8x8_t __idx)
{
#if defined(__GNUC__) && !defined(__clang__)
	return (uint8x8_t)__builtin_neon_vtbl1v8qi((int8x8_t)__tab,
	    (int8x8_t)__idx);
#elif defined(__clang__)
	uint8x8_t __ret;
#ifndef __LITTLE_ENDIAN__
	__tab = __builtin_shufflevector(__tab, __tab, 7,6,5,4,3,2,1,0);
	__idx = __builtin_shufflevector(__idx, __idx, 7,6,5,4,3,2,1,0);
#endif
	__ret = (uint8x8_t)__builtin_neon_vtbl1_v((int8x8_t)__tab,
	    (int8x8_t)__idx, 16);
#ifndef __LITTLE_ENDIAN__
	__ret = __builtin_shufflevector(__ret, __ret, 7,6,5,4,3,2,1,0);
#endif
	return __ret;
#endif
}

_INTRINSATTR
static __inline uint8x8_t
vtbl2_u8(uint8x8x2_t __tab, uint8x8_t __idx)
{
#if defined(__GNUC__) && !defined(__clang__)
	union {
		uint8x8x2_t __u8x8x82;
		__builtin_neon_ti __ti;
	} __u = { __tab };
	return (uint8x8_t)__builtin_neon_vtbl2v8qi(__u.__ti, (int8x8_t)__idx);
#elif defined(__clang__)
	uint8x8_t __ret;
#ifndef __LITTLE_ENDIAN__
	__tab.val[0] = __builtin_shufflevector(__tab.val[0], __tab.val[0],
	    7,6,5,4,3,2,1,0);
	__tab.val[1] = __builtin_shufflevector(__tab.val[1], __tab.val[1],
	    7,6,5,4,3,2,1,0);
	__idx = __builtin_shufflevector(__idx, __idx, 7,6,5,4,3,2,1,0);
#endif
	__ret = (uint8x8_t)__builtin_neon_vtbl2_v((int8x8_t)__tab.val[0],
	    (int8x8_t)__tab.val[1], (int8x8_t)__idx, 16);
#ifndef __LITTLE_ENDIAN__
	__ret = __builtin_shufflevector(__ret, __ret, 7,6,5,4,3,2,1,0);
#endif
	return __ret;
#endif
}

#endif	/* !defined(__aarch64__) */

#endif	/* _SYS_CRYPTO_CHACHA_ARCH_ARM_ARM_NEON_H */
