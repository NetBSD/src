/* $NetBSD: qp.c,v 1.4 2022/10/05 10:28:19 nia Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#include <sys/cdefs.h>

#include "milieu.h"
#include "softfloat.h"

/*
 * This file provides wrappers for the softfloat functions.  We can't use
 * invoke them directly since long double arguments are passed in FP/SIMD
 * as well as being returned in them while float128 arguments are passed
 * in normal registers.
 *
 * XXX: we're using compiler_rt for this now. Only one function remains
 * that is missing from compiler_rt.
 */

long double __negtf2(long double);

union sf_ieee_flt_u {
	float fltu_f;
	float32 fltu_f32;
};

union sf_ieee_dbl_u {
	double dblu_d;
	float64 dblu_f64;
};

union sf_ieee_ldbl_u {
	long double ldblu_ld;
	float128 ldblu_f128;
};

long double
__negtf2(long double ld_a)
{
	const union sf_ieee_ldbl_u zero = { .ldblu_ld = 0.0 };
	const union sf_ieee_ldbl_u a = { .ldblu_ld = ld_a };
	const union sf_ieee_ldbl_u b = {
	    .ldblu_f128 = float128_div(zero.ldblu_f128, a.ldblu_f128)
	};

	return b.ldblu_ld;
}
