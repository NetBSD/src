/* $NetBSD: systmodes.S,v 1.2 2001/03/13 07:41:26 ross Exp $ */

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
Sets the system's IEC/IEEE floating-point rounding mode.  Also disables all
system exception traps.
-------------------------------------------------------------------------------
*/

#include <machine/asm.h>

ENTRY(syst_float_set_rounding_mode)
	movb 4(%esp),%al
	andb $3,%al
	shlw $10,%ax
	orw $63,%ax
	subl $2,%esp
	fnstcw 0(%esp)
	andw $768,0(%esp)
	orw %ax,0(%esp)
	fldcw 0(%esp)
	addl $2,%esp
	ret

/*
-------------------------------------------------------------------------------
Sets the rounding precision of subsequent extended double-precision
operations.  The `precision' argument should be one of 0, 32, 64, or 80.
If `precision' is 32, the rounding precision is set equivalent to single
precision; else if `precision' is 64, the rounding precision is set
equivalent to double precision; else the rounding precision is set to full
extended double precision.
-------------------------------------------------------------------------------
*/

ENTRY(syst_float_set_rounding_precision)
	movb 4(%esp),%al
	movb $0,%ah
	cmpb $32,%al
	je setRoundingPrecision
	movb $2,%ah
	cmpb $64,%al
	je setRoundingPrecision
	movb $3,%ah
	cmpb $80,%al
	je setRoundingPrecision
	movb $0,%ah
setRoundingPrecision:
	movb $0,%al
	subl $2,%esp
	fnstcw 0(%esp)
	andw $64767,0(%esp)
	orw %ax,0(%esp)
	fldcw 0(%esp)
	addl $2,%esp
	ret

