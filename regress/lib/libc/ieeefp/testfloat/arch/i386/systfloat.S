/* $NetBSD: systfloat.S,v 1.2 2001/03/13 07:43:19 ross Exp $ */

/* This is a derivative work. */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
===============================================================================

This GNU assembler source file is part of TestFloat, Release 2a, a package
of programs for testing the correctness of floating-point arithmetic
complying to the IEC/IEEE Standard for Floating-Point.

Written by John R. Hauser.  More information is available through the Web
page `http://HTTP.CS.Berkeley.EDU/~jhauser/arithmetic/TestFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort
has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT
TIMES RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO
PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY
AND ALL LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these four paragraphs for those parts of
this code that are retained.

===============================================================================
*/

	.text

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

#include <machine/asm.h>

ENTRY(syst_int32_to_floatx80)
	fildl 8(%esp)
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

ENTRY(syst_int64_to_floatx80)
	fildq 8(%esp)
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_float32_to_floatx80)
	flds 8(%esp)
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_float64_to_floatx80)
	fldl 8(%esp)
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_to_int32)
	fldt 4(%esp)
	subl $4,%esp
	fistpl (%esp)
	movl (%esp),%eax
	addl $4,%esp
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_to_int64)
	fldt 4(%esp)
	subl $8,%esp
	fistpq (%esp)
	movl (%esp),%eax
	movl 4(%esp),%edx
	addl $8,%esp
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_to_float32)
	fldt 4(%esp)
	subl $4,%esp
	fstps (%esp)
	movl (%esp),%eax
	addl $4,%esp
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_to_float64)
	fldt 4(%esp)
	subl $8,%esp
	fstpl (%esp)
	movl 4(%esp),%edx
	movl (%esp),%eax
	addl $8,%esp
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_round_to_int)
	fldt 8(%esp)
	frndint
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_add)
	fldt 8(%esp)
	fldt 20(%esp)
	faddp
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_sub)
	fldt 8(%esp)
	fldt 20(%esp)
	fsubrp
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_mul)
	fldt 8(%esp)
	fldt 20(%esp)
	fmulp
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_div)
	fldt 8(%esp)
	fldt 20(%esp)
	fdivrp
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_rem)
	fldt 20(%esp)
	fldt 8(%esp)
floatx80_rem_loop:
	fprem1
	fnstsw %ax
	btw $10,%ax
	jc floatx80_rem_loop
	movl 4(%esp),%eax
	fstpt (%eax)
	fstp %st(0)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_sqrt)
	fldt 8(%esp)
	fsqrt
	movl 4(%esp),%eax
	fstpt (%eax)
	ret $4

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_eq)
	fldt 16(%esp)
	fldt 4(%esp)
	fucompp
	fnstsw %ax
	andw $17664,%ax
	cmpw $16384,%ax
	seteb %al
	movzb %al,%eax
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_le)
	fldt 4(%esp)
	fldt 16(%esp)
	fcompp
	fnstsw %ax
	notl %eax
	shrl $8,%eax
	andl $1,%eax
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_lt)
	fldt 4(%esp)
	fldt 16(%esp)
	fcompp
	fnstsw %ax
	andw $17664,%ax
	setzb %al
	movzb %al,%eax
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_eq_signaling)
	fldt 16(%esp)
	fldt 4(%esp)
	fcompp
	fnstsw %ax
	andw $17664,%ax
	cmpw $16384,%ax
	seteb %al
	movzb %al,%eax
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
ENTRY(syst_floatx80_le_quiet)
	fldt 4(%esp)
	fldt 16(%esp)
	fucompp
	fnstsw %ax
	notl %eax
	shrl $8,%eax
	andl $1,%eax
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

ENTRY(syst_floatx80_lt_quiet)
	fldt 4(%esp)
	fldt 16(%esp)
	fucompp
	fnstsw %ax
	andw $17664,%ax
	setzb %al
	movzb %al,%eax
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

ENTRY(syst_floatx80_to_int32_round_to_zero)
	pushl %ebp
	movl %esp,%ebp
	subl $12,%esp
	fldt 8(%ebp)
	fnstcw -4(%ebp)
	movl -4(%ebp),%edx
	movb $12,%dh
	movl %edx,-12(%ebp)
	fldcw -12(%ebp)
	fistpl -12(%ebp)
	movl -12(%ebp),%eax
	fldcw -4(%ebp)
	leave
	ret

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

ENTRY(syst_floatx80_to_int64_round_to_zero)
	pushl %ebp
	movl %esp,%ebp
	subl $12,%esp
	fldt 8(%ebp)
	fnstcw -4(%ebp)
	movl -4(%ebp),%ecx
	movb $12,%ch
	movl %ecx,-12(%ebp)
	fldcw -12(%ebp)
	fistpq -12(%ebp)
	movl -12(%ebp),%eax
	movl -8(%ebp),%edx
	fldcw -4(%ebp)
	leave
	ret
