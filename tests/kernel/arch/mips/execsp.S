/*	$NetBSD: execsp.S,v 1.1 2025/04/20 22:32:25 riastradh Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.1 2025/04/20 22:32:25 riastradh Exp $")

#include "SYS.h"

	.text

/*
 * void execsp_start(void *stackpointer@a0, void (*cleanup@a1)(void),
 *     void *obj_main@a2, struct ps_strings *ps_strings@a3)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
LEAF(execsp_start)
	move	t0, sp			/* save stack pointer just in case */

	PIC_PROLOGUE(execsp_start)	/* gp setup, sets t3 */

	PTR_LA	t1, _C_LABEL(startsp)	/* load t1 := &startsp */
	 NOP_L				/* load hazard */
	PTR_S	t0, 0(t1)		/* store startsp := stack pointer */

	PIC_TAILCALL(__start)		/* gp restore, uses t3 */
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
LEAF(execsp_ctor)
	move	t0, sp			/* save stack pointer just in case */

	PIC_PROLOGUE(execsp_ctor)	/* gp setup, sets t3 */

	PTR_LA	t1, _C_LABEL(ctorsp)	/* load t1 := &ctorsp */
	 NOP_L				/* load hazard */
	PTR_S	t0, 0(t1)		/* store ctorsp := stack pointer */

	PIC_RETURN()			/* gp restore, uses t3 */
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.pushsection .ctors,"aw",@progbits
	.p2align PTR_SCALESHIFT
	PTR_WORD	_C_LABEL(execsp_ctor)
	.popsection

/*
 * int main(int argc@a0, char **argv@a1, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
LEAF(main)
	move	t0, sp			/* save stack pointer just in case */

	PIC_PROLOGUE(main)		/* gp setup, sets t3 */

	PTR_LA	t1, _C_LABEL(mainsp)	/* load t1 := &mainsp */
	 NOP_L				/* load hazard */
	PTR_S	t0, 0(t1)		/* store mainsp := stack pointer */

	move	v0, zero		/* return 0 */

	PIC_RETURN()			/* gp restore, uses t3 */
END(main)

/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
LEAF(execsp_dtor)
	move	t0, sp			/* save stack pointer just in case */

	PIC_PROLOGUE(execsp_dtor)	/* gp setup, sets t3 */

	PTR_LA	t1, _C_LABEL(dtorsp)	/* load t1 := &dtorsp */
	 NOP_L				/* load hazard */
	PTR_S	t0, 0(t1)		/* store dtorsp := stack pointer */

	PIC_TAILCALL(execsp_main)	/* gp restore, uses t3 */
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.pushsection .dtors,"aw",@progbits
	.p2align PTR_SCALESHIFT
	PTR_WORD	_C_LABEL(execsp_dtor)
	.popsection
