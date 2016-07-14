/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

RCSID("$NetBSD: fpsf.S,v 1.1 2016/07/14 01:59:18 matt Exp $")

/*
 * This file provides softfloat compatible routines which use FP instructions
 * to do the actual work.  This should give near hard-float performance while
 * being compatible with soft-float code.
 *
 * This file implements the single precision floating point routines.
 */

#ifdef MIPS3
#define	COP1_SYNC	nop
#else
#define	COP1_SYNC
#endif

LEAF_NOPROFILE(__addsf3)
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	add.s		$f0, $f12, $f14
	mfc1		v0, $f0
	jr		ra
END(__addsf3)

LEAF_NOPROFILE(__subsf3)
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	sub.s		$f0, $f12, $f14
	mfc1		v0, $f0
	jr		ra
END(__subsf3)

LEAF_NOPROFILE(__mulsf3)
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	mul.s		$f0, $f12, $f14
	mfc1		v0, $f0
	jr		ra
END(__mulsf3)

LEAF_NOPROFILE(__divsf3)
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	div.s		$f0, $f12, $f14
	mfc1		v0, $f0
	jr		ra
END(__divsf3)

LEAF_NOPROFILE(__negsf2)
	mtc1		a0, $f12
	COP1_SYNC
	neg.s		$f0, $f12
	mfc1		v0, $f0
	jr		ra
END(__negsf2)

LEAF_NOPROFILE(__truncdfsf2)
	dmtc1		a0, $f12
	COP1_SYNC
	cvt.s.d		$f0, $f12
	mfc1		v0, $f0
	jr		ra
END(__truncdfsf2)

LEAF_NOPROFILE(__fixsfdi)
	mtc1		a0, $f12
	COP1_SYNC
	trunc.l.s	$f0, $f12
	dmfc1		v0, $f0
	jr		ra
END(__fixsfdi)

LEAF_NOPROFILE(__fixsfsi)
	mtc1		a0, $f12
	COP1_SYNC
	trunc.w.s	$f0, $f12
	mfc1		v0, $f0
	jr		ra
END(__fixsfsi)

LEAF_NOPROFILE(__fixunssfdi)
	lui		t0, 0x5f00	# 9223372036854775808.0
	mtc1		t0, $f0
	mtc1		a0, $f12
	COP1_SYNC
	sub.s		$f0, $f12, $f0	# convert to signed 
        trunc.l.s	$f0, $f0
	dmfc1		v0, $f0
	li		v1, 1
	dsll		v1, v1, 63
	add		v0, v0, v1	# convert to unsigned
	jr		ra
END(__fixunssfdi)

LEAF_NOPROFILE(__fixunssfsi)
	lui		t0, 0x4f00	# 2147483648.0
	mtc1		t0, $f0
	mtc1		a0, $f12
	COP1_SYNC
	sub.s		$f0, $f12, $f0	# convert to signed 
        trunc.w.s	$f0, $f0
	mfc1		v0, $f0
	lui		v1, 0x8000	# 0xffffffff80000000
	add		v0, v0, v1	# convert to unsigned
	jr		ra
END(__fixunssfsi)

LEAF_NOPROFILE(__floatdisf)
	dmtc1		a0, $f12
	COP1_SYNC
	cvt.s.l		$f0, $f12
	mfc1		v0, $f0
	jr		ra
END(__floatdisf)

LEAF_NOPROFILE(__floatsisf)
	mtc1		a0, $f12
	COP1_SYNC
	cvt.s.w		$f0, $f12
	mfc1		v0, $f0
	jr		ra
END(__floatsisf)

LEAF_NOPROFILE(__floatundisf)
	li		t0, 1
	dsll		t0, t0, 63	# 9223372036854775808.0
	dsub		a0, a0, t0	# convert to signed
	dmtc1		a0, $f12
	dmtc1		t0, $f14
	COP1_SYNC
	cvt.s.l		$f0, $f12
	cvt.s.l		$f2, $f14
	add.s		$f0, $f0, $f2	# convert back to unsigned
	mfc1		v0, $f0 
	jr		ra
END(__floatundisf)

LEAF_NOPROFILE(__floatunsisf)
	sll		a0, a0, 0	# sign extend to 64 bits
	dmtc1		a0, $f12
	COP1_SYNC
	cvt.s.l		$f0, $f12
	mfc1		v0, $f0 
	jr		ra
END(__floatunsisf)

STRONG_ALIAS(__eqsf2, __nedf2)
LEAF_NOPROFILE(__nesf2)
	.set push
	.set noreorder
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	c.eq.s		$f12, $f14
	bc1f		2f
	 li		v0, 1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__nesf2)

STRONG_ALIAS(__gesf2, __ltdf2)
LEAF_NOPROFILE(__ltsf2)
	.set push
	.set noreorder
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	c.olt.s		$f12, $f14
	bc1t		2f
	 li		v0, -1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__ltsf2)

STRONG_ALIAS(__gtsf2, __ledf2)
LEAF_NOPROFILE(__lesf2)
	.set push
	.set noreorder
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	c.ole.s		$f12, $f14
	bc1f		2f
	 li		v0, 1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__lesf2)

LEAF_NOPROFILE(__unordsf2)
	.set push
	.set noreorder
	mtc1		a0, $f12
	mtc1		a1, $f14
	COP1_SYNC
	c.un.s		$f12, $f14
	bc1t		2f
	 li		v0, 1
	move		v0, zero
2:	jr		ra
	 nop
	.set pop
END(__unordsf2)
