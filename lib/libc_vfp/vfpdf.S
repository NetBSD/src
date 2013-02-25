/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <arm/asm.h>

RCSID("$NetBSD: vfpdf.S,v 1.1.4.2 2013/02/25 00:27:55 tls Exp $")

/*
 * This file provides softfloat compatible routines which use VFP instructions
 * to do the actual work.  This should give near hard-float performance while
 * being compatible with soft-float code.
 *
 * This file implements the double precision floating point routines.
 */

#ifdef	__ARMEL__
#define	vmov_arg0	vmov	d0, r0, r1
#define	vmov_arg1	vmov	d1, r2, r3
#define	vmov_ret	vmov	r0, r1, d0
#else
#define	vmov_arg0	vmov	d0, r1, r0
#define	vmov_arg1	vmov	d1, r3, r2
#define	vmov_ret	vmov	r1, r0, d0
#endif
#define	vmov_args	vmov_arg0; vmov_arg1

ENTRY(__adddf3)
	vmov_args
	vadd.f64	d0, d0, d1
	vmov_ret
	RET
END(__adddf3)

ENTRY(__subdf3)
	vmov_args
	vsub.f64	d0, d0, d1
	vmov_ret
	RET
END(__subdf3)

ENTRY(__muldf3)
	vmov_args
	vmul.f64	d0, d0, d1
	vmov_ret
	RET
END(__muldf3)

ENTRY(__divdf3)
	vmov_args
	vdiv.f64	d0, d0, d1
	vmov_ret
	RET
END(__divdf3)

ENTRY(__negdf2)
	vmov_arg0
	vneg.f64	d0, d0
	vmov_ret
	RET
END(__negdf2)

ENTRY(__extendsfdf2)
	vmov		s0, r0
	vcvt.f64.f32	d0, s0
	vmov_ret
	RET
END(__extendsfdf2)

ENTRY(__fixdfsi)
	vmov_arg0
	vcvt.s32.f64	s0, d0
	vmov		r0, s0
	RET
END(__fixdfsi)

ENTRY(__fixunsdfsi)
	vmov_arg0
	vcvt.u32.f64	s0, d0
	vmov		r0, s0
	RET
END(__fixunsdfsi)

ENTRY(__floatsidf)
	vmov		s0, r0
	vcvt.f64.s32	d0, s0
	vmov_ret
	RET
END(__floatsidf)

ENTRY(__floatunsidf)
	vmov		s0, r0
	vcvt.f64.u32	d0, s0
	vmov_ret
	RET
END(__floatunsidf)

/* N set if compare <= result */
/* Z set if compare = result */
/* C set if compare (=,>=,UNORD) result */
/* V set if compare UNORD result */

STRONG_ALIAS(__eqdf2, __nedf2)
ENTRY(__nedf2)
	vmov_args
	vcmp.f64	d0, d1
	vmrs		APSR_nzcv, fpscr
	moveq		r0, #0		/* !(a == b) */
	movne		r0, #1		/* !(a == b) */
	RET
END(__nedf2)

STRONG_ALIAS(__gedf2, __ltdf2)
ENTRY(__ltdf2)
	vmov_args
	vcmp.f64	d0, d1
	vmrs		APSR_nzcv, fpscr
	mvnmi		r0, #0		/* -(a < b) */
	movpl		r0, #0		/* -(a < b) */
	RET
END(__ltdf2)

STRONG_ALIAS(__gtdf2, __ledf2)
ENTRY(__ledf2)
	vmov_args
	vcmp.f64	d0, d1
	vmrs		APSR_nzcv, fpscr
	movgt		r0, #1		/* (a > b) */
	movle		r0, #0		/* (a > b) */
	RET
END(__ledf2)

ENTRY(__unorddf2)
	vmov_args
	vcmp.f64	d0, d1
	vmrs		APSR_nzcv, fpscr
	movvs		r0, #1		/* isnan(a) || isnan(b) */
	movvc		r0, #0		/* isnan(a) || isnan(b) */
	RET
END(__unorddf2)
