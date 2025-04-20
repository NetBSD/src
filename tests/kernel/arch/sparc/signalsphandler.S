/*	$NetBSD: signalsphandler.S,v 1.1 2025/04/20 22:33:41 riastradh Exp $	*/

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

RCSID("$NetBSD: signalsphandler.S,v 1.1 2025/04/20 22:33:41 riastradh Exp $")

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
 * signalsphandler(signo@o0)
 *
 *	Signal handler.  Store the stack pointer on entry at the global
 *	variable signalsp and return.
 */
ENTRY(signalsphandler)
	/* Reminder: o6 is frame pointer, o7 + 8 is return address. */
	PIC_PROLOGUE(%g1, %o5)			/* g1 := &GOT, clobber o5 */
	SETHI(_C_LABEL(signalsp), %g1, %o5)	/* o5 := &signalsp */
	retl					/* return to caller */
	 st	%sp, [LO(_C_LABEL(signalsp), %o5)] /* signalsp := sp */
END(signalsphandler)
