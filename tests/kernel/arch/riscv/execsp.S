/*	$NetBSD: execsp.S,v 1.1 2025/04/20 22:34:07 riastradh Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.1 2025/04/20 22:34:07 riastradh Exp $")

/*
 * void execsp_start(void (*cleanup@a0)(void),
 *     struct ps_strings *ps_strings@a1)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
	.option push
	.option norelax		/* Don't optimize this into `mv gp, gp'! */
	lla	gp, __global_pointer$
	.option pop

	PTR_LA	t0, _C_LABEL(startsp)	/* t0 := &startsp */
	PTR_S	sp, 0(t0)		/* startsp := sp */
	tail	_C_LABEL(__start)	/* tail call __start */
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	PTR_LA	t0, _C_LABEL(ctorsp)	/* t0 := &ctorsp */
	PTR_S	sp, 0(t0)		/* ctorsp := sp */
	ret
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.section .init_array,"aw"
	.p2align PTR_SCALESHIFT
	PTR_WORD	_C_LABEL(execsp_ctor)

/*
 * int main(int argc@a0, char **argv@a1, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	PTR_LA	t0, _C_LABEL(mainsp)	/* t0 := &mainsp */
	PTR_S	sp, 0(t0)		/* mainsp := sp */
	li	a0, 0			/* return 0 */
	ret
END(main)

/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
ENTRY(execsp_dtor)
	PTR_LA	t0, _C_LABEL(dtorsp)	/* t0 := &dtorsp */
	PTR_S	sp, 0(t0)		/* dtorsp := sp */
	tail	_C_LABEL(execsp_main)	/* tail call execsp_main */
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.section .fini_array,"aw"
	.p2align PTR_SCALESHIFT
	PTR_WORD	_C_LABEL(execsp_dtor)
