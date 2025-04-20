/*	$NetBSD: execsp.S,v 1.1 2025/04/20 22:33:13 riastradh Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.1 2025/04/20 22:33:13 riastradh Exp $")

/*
 * void execsp_start(void (*cleanup@edx)(void), void *obj_main@ecx,
 *     struct ps_strings *ps_strings@ebx)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
	call	getpc_eax
	addl	$_GLOBAL_OFFSET_TABLE_,%eax
	movl	_C_LABEL(startsp)@GOT(%eax),%eax
	movl	%esp,(%eax)
	jmp	_C_LABEL(__start)@PLT
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	call	getpc_eax
	addl	$_GLOBAL_OFFSET_TABLE_,%eax
	movl	_C_LABEL(ctorsp)@GOT(%eax),%eax
	movl	%esp,(%eax)
	ret
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.section .ctors,"aw",@progbits
	.p2align 2
	.long	_C_LABEL(execsp_ctor)

/*
 * int main(int argc@rdi, char **argv@rsi, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	call	getpc_eax
	addl	$_GLOBAL_OFFSET_TABLE_,%eax
	movl	_C_LABEL(mainsp)@GOT(%eax),%eax
	movl	%esp,(%eax)
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
	call	getpc_eax
	addl	$_GLOBAL_OFFSET_TABLE_,%eax
	movl	_C_LABEL(dtorsp)@GOT(%eax),%eax
	movl	%esp,(%eax)
	jmp	_C_LABEL(execsp_main)@PLT
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.section .dtors,"aw",@progbits
	.p2align 2
	.long	_C_LABEL(execsp_dtor)

	.text
	_ALIGN_TEXT
	.local	getpc_eax
	.type	getpc_eax,@function
getpc_eax:
	movl	(%esp),%eax
	ret
END(getpc_eax)
