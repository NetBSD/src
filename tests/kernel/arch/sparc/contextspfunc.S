/*	$NetBSD: contextspfunc.S,v 1.1 2025/04/21 02:33:45 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#define	_LOCORE

#include <machine/asm.h>

RCSID("$NetBSD: contextspfunc.S,v 1.1 2025/04/21 02:33:45 riastradh Exp $")

#ifdef __PIC__
#define	SETHI(label, got, reg)						      \
	set	label, reg;		/* reg := &label - &GOT */	      \
	ld	[got + reg], reg	/* reg := &label */
#define	LO(label, reg)							      \
	reg
#else
#define	SETHI(label, got, reg)						      \
	sethi	%hi(label), reg		/* reg := &label - %lo(label) */
#define	LO(label, reg)							      \
	reg + %lo(label)
#endif

	.text

/*
 * contextspfunc()
 *
 *	makecontext(3) function.  Store the stack pointer on entry at
 *	the global variable contextsp and call contextdone.
 */
ENTRY(contextspfunc)
	/* Reminder: o6 is frame pointer, o7 + 8 is return address. */
	PIC_PROLOGUE(%g1, %o5)			/* g1 := &GOT, clobber o5 */
	SETHI(_C_LABEL(contextsp), %g1, %o5)	/* o5 := &contextsp */
	call	_C_LABEL(contextdone)		/* jump to contextdone */
	 st	%sp, [LO(_C_LABEL(contextsp), %o5)] /* contextsp := sp */
	/* don't care what happens here, caller must never return */
	ta	1				/* Tcc, trap always */
END(contextspfunc)
