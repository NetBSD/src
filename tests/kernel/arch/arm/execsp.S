/*	$NetBSD: execsp.S,v 1.4 2025/05/07 16:26:47 uwe Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.4 2025/05/07 16:26:47 uwe Exp $")

/*
 * void execsp_start(struct ps_strings *ps_strings@r0, void *obj_main@r1,
 *     void (*cleanup@r2)(void))
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
0:	GOT_INIT(r3, .Lgot.execsp_start)
	mov	r4, sp
	GOT_GET(r5, r3, .Lstartsp)
	str	r4, [r5]
	b	PLT_SYM(_C_LABEL(__start))

	GOT_INITSYM(.Lgot.execsp_start, 0b)
.Lstartsp:
	.word	GOT_SYM(startsp)
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
0:	GOT_INIT(r0, .Lgot.execsp_ctor)
	mov	r1, sp
	GOT_GET(r2, r0, .Lctorsp)
	str	r1, [r2]
	RET

	GOT_INITSYM(.Lgot.execsp_ctor, 0b)
.Lctorsp:
	.word	GOT_SYM(ctorsp)
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.section .init_array,"aw",%init_array
	.p2align 2
	.word	_C_LABEL(execsp_ctor)

/*
 * int main(int argc@r0, char **argv@r1, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
0:	GOT_INIT(r0, .Lgot.main)
	mov	r1, sp
	GOT_GET(r2, r0, .Lmainsp)
	str	r1, [r2]
	mov	r0, #0
	RET

	GOT_INITSYM(.Lgot.main, 0b)
.Lmainsp:
	.word	GOT_SYM(mainsp)
END(main)

/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
ENTRY(execsp_dtor)
0:	GOT_INIT(r0, .Lgot.execsp_dtor)
	mov	r1, sp
	GOT_GET(r2, r0, .Ldtorsp)
	str	r1, [r2]
	b	PLT_SYM(_C_LABEL(execsp_main))

	GOT_INITSYM(.Lgot.execsp_dtor, 0b)
.Ldtorsp:
	.word	GOT_SYM(dtorsp)
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.section .fini_array,"aw",%fini_array
	.p2align 2
	.word	_C_LABEL(execsp_dtor)
