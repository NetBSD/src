/* $NetBSD: rtld_start.S,v 1.2 2018/02/04 21:49:51 skrll Exp $ */

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

#include <machine/asm.h>

RCSID("$NetBSD: rtld_start.S,v 1.2 2018/02/04 21:49:51 skrll Exp $")

/*
 * void _rtld_start(void (*cleanup)(void), const Obj_Entry *obj,
 *    struct ps_strings *ps_strings);
 *
 * X0		= NULL
 * X1		= NULL
 * X2		= ps_strings
 * X30 (LR)	= 0
 * X29 (FP)	= 0
 */
ENTRY_NP(_rtld_start)
	mov	x24, x2			/* save ps_strings */

	adrp	x1, :got:_DYNAMIC	/* load _DYNAMIC offset from GOT */
	ldr	x1, [x1, #:got_lo12:_DYNAMIC]

	adrp	x0, _DYNAMIC		/* get &_DYNAMIC */
	add	x0, x0, #:lo12:_DYNAMIC

	sub	x25, x0, x1		/* relocbase = &_DYNAMIC - GOT:_DYNAMIC */
	mov	x1, x25			/* pass as 2nd argument */
	bl	_C_LABEL(_rtld_relocate_nonplt_self)

	sub	sp, sp, #16		/* reserve space for returns */
	mov	x0, sp			/* pointer to reserved space */
	mov	x1, x25			/* pass relocbase */
	bl	_C_LABEL(_rtld)
	mov	x17, x0			/* save entry point */

	ldp	x0, x1, [sp], #16	/* pop cleanup & obj_main */
	mov	x2, x24			/* restore ps_strings */

	br	x17			/* call saved entry point */
END(_rtld_start)

/*
 * Upon entry from plt0 entry:
 *
 * SP+0		= &PLTGOT[n + 3]
 * SP+8		= return addr
 * X16		= &PLTGOT[2]
 */
ENTRY_NP(_rtld_bind_start)
	ldr	x9, [sp]		/* x9 = &PLTGOT[n+3] */

	/* save x0-x8 for arguments */
	stp	x0, x1, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x6, x7, [sp, #-16]!
	stp	x8, xzr, [sp, #-16]!

	/* save q0-q7 for arguments */
	stp	q0, q1, [sp, #-32]!
	stp	q2, q3, [sp, #-32]!
	stp	q4, q5, [sp, #-32]!
	stp	q6, q7, [sp, #-32]!

	ldr	x0, [x16, #-8]	/* x0 = PLTGOT[1] */
	sub	x1, x9, x16	/* x1 = &PLTGOT[n+3] - &PLTGOT[1] = offset+8 */
	sub	x1, x1, #8	/* x1 = offset */
	lsr	x1, x1, #3	/* x1 /= sizeof(void *) */

	bl	_C_LABEL(_rtld_bind)
	mov	x17, x0		/* save result */

	/* restore q0-q7 for arguments */
	ldp	q6, q7, [sp], #32
	ldp	q4, q5, [sp], #32
	ldp	q2, q3, [sp], #32
	ldp	q0, q1, [sp], #32

	/* restore x0-x8 for arguments */
	ldp	x8, xzr, [sp], #16
	ldp	x6, x7, [sp], #16
	ldp	x4, x5, [sp], #16
	ldp	x2, x3, [sp], #16
	ldp	x0, x1, [sp], #16

	ldp	xzr, lr, [sp], #16	/* restore original lr pushed by plt0 */
	br	x17			/* call bound function */
END(_rtld_bind_start)


ENTRY(_rtld_tlsdesc)
	ldr	x0, [x0, #8]
	ret
END(_rtld_tlsdesc)
