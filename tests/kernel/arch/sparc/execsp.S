/*	$NetBSD: execsp.S,v 1.1 2025/04/20 22:33:41 riastradh Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.1 2025/04/20 22:33:41 riastradh Exp $")

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
 * void execsp_start(struct ps_strings *ps_strings@g1,
 *     void *obj_main@g2, void (*cleanup@g3)(void))
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
	PIC_PROLOGUE(%o1, %o2)			/* o1 := GOT, clobber o2 */
	SETHI(_C_LABEL(startsp), %o1, %o2)	/* o2 := &startup */
	call	_C_LABEL(__start)		/* jump to start via PLT */
	 st	%sp, [LO(_C_LABEL(startsp), %o2)] /* startsp := sp */
	/* don't care what happens here, caller must never return */
	ta	1				/* Tcc, trap always */
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	/* Reminder: o6 is frame pointer, o7 + 8 is return address. */
	PIC_PROLOGUE(%g1, %o5)			/* g1 := &GOT, clobber o5 */
	SETHI(_C_LABEL(ctorsp), %g1, %o5)	/* o5 := &ctorsp */
	retl					/* return to caller */
	 st	%sp, [LO(_C_LABEL(ctorsp), %o5)] /* ctorsp := sp */
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.pushsection .ctors,"aw",@progbits
	.p2align 2
	.long	_C_LABEL(execsp_ctor)
	.popsection

/*
 * int main(int argc@a0, char **argv@a1, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	/* Reminder: o6 is frame pointer, o7 + 8 is return address. */
	PIC_PROLOGUE(%g1, %o5)			/* g1 := &GOT, clobber o5 */
	SETHI(_C_LABEL(mainsp), %g1, %o5)	/* o5 := &mainsp */
	st	%sp, [LO(_C_LABEL(mainsp), %o5)] /* mainsp := sp */
	retl					/* return to caller */
	 mov	0, %o0				/* return 0 */
END(main)

/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
ENTRY(execsp_dtor)
	/* Reminder: o6 is frame pointer, o7 + 8 is return address. */
	PIC_PROLOGUE(%g1, %o5)			/* g1 := &GOT, clobber o5 */
	SETHI(_C_LABEL(dtorsp), %g1, %o5)	/* o5 := &dtorsp - &GOT */
	st	%sp, [LO(_C_LABEL(dtorsp), %o5)] /* dtorsp := sp */
	mov	%o7, %o5			/* save return address */
	call	_C_LABEL(execsp_main)		/* tail call to execsp_main */
	 mov	%o5, %o7			/* restore return address */
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.pushsection .dtors,"aw",@progbits
	.p2align 2
	.long	_C_LABEL(execsp_dtor)
	.popsection
