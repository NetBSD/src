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

#include <mips/asm.h>

RCSID("$NetBSD: fpdf.S,v 1.1 2016/07/14 01:59:18 matt Exp $")

/*
 * This file provides softfloat compatible routines which use FP instructions
 * to do the actual work.  This should give near hard-float performance while
 * being compatible with soft-float code.
 *
 * This file implements the double precision floating point routines.
 */

#ifdef MIPS3
#define	COP1_SYNC	nop
#else
#define	COP1_SYNC
#endif

LEAF_NOPROFILE(__adddf3)
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	add.d		$f0, $f12, $f14
	dmfc1		v0, $f0
	jr		ra
END(__adddf3)

LEAF_NOPROFILE(__subdf3)
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	sub.d		$f0, $f12, $f14
	dmfc1		v0, $f0
	jr		ra
END(__subdf3)

LEAF_NOPROFILE(__muldf3)
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	mul.d		$f0, $f12, $f14
	dmfc1		v0, $f0
	jr		ra
END(__muldf3)

LEAF_NOPROFILE(__divdf3)
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	div.d		$f0, $f12, $f14
	dmfc1		v0, $f0
	jr		ra
END(__divdf3)

LEAF_NOPROFILE(__negdf2)
	dmtc1		a0, $f12
	COP1_SYNC
	neg.d		$f0, $f12
	dmfc1		v0, $f0
	jr		ra
END(__negdf2)

LEAF_NOPROFILE(__extendsfdf2)
	dmtc1		a0, $f12
	COP1_SYNC
	cvt.d.s		$f0, $f12
	dmfc1		v0, $f0
	jr		ra
END(__extendsfdf2)

LEAF_NOPROFILE(__fixdfdi)
	dmtc1		a0, $f12
	COP1_SYNC
	trunc.l.d	$f0, $f12
	dmfc1		v0, $f0
	jr		ra
END(__fixdfdi)

LEAF_NOPROFILE(__fixdfsi)
	dmtc1		a0, $f12
	COP1_SYNC
	trunc.w.d	$f0, $f12
	mfc1		v0, $f0
	jr		ra
END(__fixdfsi)

LEAF_NOPROFILE(__fixunsdfdi)
	lui		t0, 0x43e0	# 9223372036854775808.0
	dsll		t0, t0, 32
	mtc1		t0, $f0
	mtc1		a0, $f12
	COP1_SYNC
	sub.d		$f0, $f12, $f0	# convert to signed 
        trunc.l.d	$f0, $f0
	dmfc1		v0, $f0
	li		v1, 1
	dsll		v1, v1, 63
	add		v0, v0, v1	# convert to unsigned
	jr		ra
END(__fixunsdfdi)

LEAF_NOPROFILE(__fixunsdfsi)
	lui		t0, 0x41e0	# 2147483648.0
	dsll		t0, t0, 32
	mtc1		t0, $f0
	mtc1		a0, $f12
	COP1_SYNC
	sub.d		$f0, $f12, $f0	# convert to signed 
        trunc.w.d	$f0, $f0
	lui		v1, 0x8000	# 0xffffffff80000000
	mfc1		v0, $f0
	add		v0, v0, v1	# convert to unsigned
	jr		ra
END(__fixunsdfsi)

LEAF_NOPROFILE(__floatdidf)
	dmtc1		a0, $f12
	COP1_SYNC
	cvt.d.l		$f0, $f12
	dmfc1		v0, $f0
	jr		ra
END(__floatdidf)

LEAF_NOPROFILE(__floatsidf)
	mtc1		a0, $f12
	COP1_SYNC
	cvt.d.w		$f0, $f12
	dmfc1		v0, $f0
	jr		ra
END(__floatsidf)

LEAF_NOPROFILE(__floatundidf)
	li		t0, 1
	dsll		t0, t0, 63
	dsub		a0, a0, t0
	dmtc1		a0, $f12
	dmtc1		t0, $f14
	COP1_SYNC
	cvt.d.l		$f0, $f12
	cvt.d.l		$f2, $f14
	add.d		$f0, $f0, $f2
	dmfc1		v0, $f0 
	jr		ra
END(__floatundidf)

LEAF_NOPROFILE(__floatunsidf)
	sll		a0, a0, 0
	dmtc1		a0, $f12
	COP1_SYNC
	cvt.d.l		$f0, $f12
	dmfc1		v0, $f0 
	jr		ra
END(__floatunsidf)

STRONG_ALIAS(__eqdf2, __nedf2)
LEAF_NOPROFILE(__nedf2)
	.set push
	.set noreorder
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	c.eq.d		$f12, $f14
	bc1f		2f
	 li		v0, 1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__nedf2)

STRONG_ALIAS(__gedf2, __ltdf2)
LEAF_NOPROFILE(__ltdf2)
	.set push
	.set noreorder
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	c.olt.d		$f12, $f14
	bc1t		2f
	 li		v0, -1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__ltdf2)

STRONG_ALIAS(__gtdf2, __ledf2)
LEAF_NOPROFILE(__ledf2)
	.set push
	.set noreorder
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	c.ole.d		$f12, $f14
	bc1f		2f
	 li		v0, 1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__ledf2)

LEAF_NOPROFILE(__unorddf2)
	.set push
	.set noreorder
	dmtc1		a0, $f12
	dmtc1		a1, $f14
	COP1_SYNC
	c.un.d		$f12, $f14
	bc1t		2f
	 li		v0, 1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__unorddf2)
