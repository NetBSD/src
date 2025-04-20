/*	$NetBSD: execsp.S,v 1.2 2025/04/20 22:31:25 riastradh Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.2 2025/04/20 22:31:25 riastradh Exp $")

/*
 * void execsp_start(void (*cleanup@x0)(void), void *obj_main@x1,
 *     struct ps_strings *ps_strings@x2)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
	mov	x16, sp
	adrp	x17, :pg_hi21:_C_LABEL(startsp)
	str	x16, [x17, #:lo12:_C_LABEL(startsp)]
	b	_C_LABEL(__start)
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	mov	x16, sp
	adrp	x17, :pg_hi21:_C_LABEL(ctorsp)
	str	x16, [x17, #:lo12:_C_LABEL(ctorsp)]
	ret
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.section .init_array,"aw",%init_array
	.p2align 3
	.xword	_C_LABEL(execsp_ctor)

/*
 * int main(int argc@x0, char **argv@x1, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	mov	x16, sp
	adrp	x17, :pg_hi21:_C_LABEL(mainsp)
	str	x16, [x17, #:lo12:_C_LABEL(mainsp)]
	mov	x0, #0
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
	mov	x16, sp
	adrp	x17, :pg_hi21:_C_LABEL(dtorsp)
	str	x16, [x17, #:lo12:_C_LABEL(dtorsp)]
	b	_C_LABEL(execsp_main)
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.section .fini_array,"aw",%fini_array
	.p2align 3
	.xword	_C_LABEL(execsp_dtor)
