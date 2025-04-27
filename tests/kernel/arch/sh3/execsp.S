/*	$NetBSD: execsp.S,v 1.2 2025/04/27 00:03:46 riastradh Exp $	*/

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

#include "asm.h"

RCSID("$NetBSD: execsp.S,v 1.2 2025/04/27 00:03:46 riastradh Exp $")


/*
 * void execsp_start()
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 *
 *	See sys/arch/sh3/sh3/sh3_machdep.c setregs()
 */
ENTRY(execsp_start)
	mov	sp, r1	// for consistency, don't need to
	PIC_PROLOGUE_NOSAVE(.L_start_GOT)

	MOVL_VAR(.L_startsp, r0)
	mov.l	r1, @r0

	mov.l	.L___start, r0
1:	JUMP	r0
	 nop

	.p2align 2
.L_start_GOT:	PIC_GOT_DATUM
.L_startsp:	VAR_DATUM(startsp)
.L___start:	CALL_DATUM(__start, 1b)
	SET_ENTRY_SIZE(execsp_start)


/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	mov	sp, r1
	PIC_PROLOGUE(.L_ctor_GOT)

	MOVL_VAR(.L_ctorsp, r0)
	mov.l	r1, @r0

	rts
	 PIC_EPILOGUE_SLOT

	.p2align 2
.L_ctor_GOT:	PIC_GOT_DATUM
.L_ctorsp:	VAR_DATUM(ctorsp)
	SET_ENTRY_SIZE(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.pushsection .ctors, "aw", @progbits
	.p2align 2
	.long	_C_LABEL(execsp_ctor)
	.popsection


/*
 * int main(int argc, char **argv, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	mov	sp, r1
	PIC_PROLOGUE(.L_main_GOT)

	MOVL_VAR(.L_mainsp, r0)
	mov.l	r1, @r0

	PIC_EPILOGUE
	rts
	 mov	#0, r0

	.p2align 2
.L_main_GOT:	PIC_GOT_DATUM
.L_mainsp:	VAR_DATUM(mainsp)
	SET_ENTRY_SIZE(main)


/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
ENTRY(execsp_dtor)
	mov	sp, r1
	sts.l	pr, @-sp
	PIC_PROLOGUE(.L_dtor_GOT)

	MOVL_VAR(.L_dtorsp, r0)
	mov.l	r1, @r0

	mov.l	.L_execsp_main, r0
1:	CALL	r0
	 nop

	PIC_EPILOGUE
	lds.l	@sp+, pr
	rts
	 nop

	.p2align 2
.L_dtor_GOT:	PIC_GOT_DATUM
.L_dtorsp:	VAR_DATUM(dtorsp)
.L_execsp_main:	CALL_DATUM(execsp_main, 1b)
	SET_ENTRY_SIZE(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.pushsection .dtors, "aw", @progbits
	.p2align 2
	.long	_C_LABEL(execsp_dtor)
	.popsection
