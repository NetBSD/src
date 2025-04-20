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
 * void execsp_start(void (*cleanup@rbx)(void), void *obj_main@rcx,
 *     struct ps_strings *ps_strings@rbx)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
	movq	%rsp,_C_LABEL(startsp)(%rip)
	/*
	 * No adjustment like in main because entry point is special
	 * and the amd64 csu __start routine takes care of it.
	 *
	 * XXX Why don't we just arrange to align it in the kernel
	 * anyway?
	 */
	jmp	_C_LABEL(__start)
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	/*
	 * `The end of the input argument area shall be aligned on a
	 *  [16-byte] boundary.  In other words, the value of (%rsp + 8)
	 *  is always a multiple of 16 when control is transferred to
	 *  the function entry point.'
	 *
	 * To make it convenient for t_signal_and_sp.c, we subtract 8
	 * from %rsp in order to get something congruent to zero modulo
	 * the stack alignemnt.
	 */
	movq	%rsp,_C_LABEL(ctorsp)(%rip)
	addq	$-8,_C_LABEL(ctorsp)(%rip)
	ret
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.section .ctors,"aw",@progbits
	.p2align 3
	.quad	_C_LABEL(execsp_ctor)

/*
 * int main(int argc@rdi, char **argv@rsi, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	movq	%rsp,_C_LABEL(mainsp)(%rip)
	addq	$-8,_C_LABEL(mainsp)(%rip)
	xorl	%eax,%eax
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
	movq	%rsp,_C_LABEL(dtorsp)(%rip)
	addq	$-8,_C_LABEL(dtorsp)(%rip)
	jmp	_C_LABEL(execsp_main)
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.section .dtors,"aw",@progbits
	.p2align 3
	.quad	_C_LABEL(execsp_dtor)
