/*	$NetBSD: arm_neon_imm.h,v 1.2 2020/08/09 01:59:04 riastradh Exp $	*/

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

#ifndef	_SYS_CRYPTO_CHACHA_ARCH_ARM_ARM_NEON_IMM_H
#define	_SYS_CRYPTO_CHACHA_ARCH_ARM_ARM_NEON_IMM_H

/*
 * Non-standard macros for writing ARM NEON vector literals.  Needed
 * because apparently GCC and Clang disagree wildly on the ordering of
 * vector literal components -- and both disagree with the
 * architectural indexing!
 */

#if defined(__GNUC__) && !defined(__clang__)
#if defined(__AARCH64EB__)
#define	V_N_U8(a,b,c,d,e,f,g,h)						      \
	{h,g,f,e,d,c,b,a}
#define	VQ_N_U8(a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p)			      \
	{p,o,n,m,l,k,j,i, h,g,f,e,d,c,b,a}
#define	VQ_N_U32(a,b,c,d)						      \
	{d,c, b,a}
#elif defined(__ARM_BIG_ENDIAN)
#define	V_N_U8(a,b,c,d,e,f,g,h)						      \
	{h,g,f,e,d,c,b,a}
#define	VQ_N_U8(a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p)			      \
	{h,g,f,e,d,c,b,a, p,o,n,m,l,k,j,i}
#define	VQ_N_U32(a,b,c,d)						      \
	{b,a, d,c}
#else
#define	V_N_U8(a,b,c,d,e,f,g,h)						      \
	{a,b,c,d,e,f,g,h}
#define	VQ_N_U8(a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p)			      \
	{a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p}
#define	VQ_N_U32(a,b,c,d)						      \
	{a,b, c,d}
#endif
#elif defined(__clang__)
#ifdef __LITTLE_ENDIAN__
#define	V_N_U8(a,b,c,d,e,f,g,h)						      \
	{a,b,c,d,e,f,g,h}
#define	VQ_N_U8(a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p)			      \
	{a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p}
#define	VQ_N_U32(a,b,c,d)						      \
	{a,b, c,d}
#else
#define	V_N_U8(a,b,c,d,e,f,g,h)						      \
	{h,g,f,e,d,c,b,a}
#define	VQ_N_U8(a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p)			      \
	{p,o,n,m,l,k,j,i, h,g,f,e,d,c,b,a}
#define	VQ_N_U32(a,b,c,d)						      \
	{d,c, b,a}
#endif
#endif

#endif	/* _SYS_CRYPTO_CHACHA_ARCH_ARM_ARM_NEON_IMM_H */
